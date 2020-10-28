# Advanced Shell

# Managing processes

If a command is not a built-in, then its first token names an external
program to execute.  The program executes in a child process, grouped
with any other child processes that are part of the same job.  The
essence of this lab is to use the basic Unix system calls to manage
processes and their execution.

## Fork

Fork system call creates a copy of itself in a separate address
space. The original process that calls fork() system call is called a
parent process and the newly created process is called a child
process.

We given you some starter code in a procedure called spawn\_job that
uses the fork system call to create a child process for a job.
The spawn\_job routine also attends to a few other details.  

A job has multiple processes if it is a pipeline, i.e., a sequence of
commands linked by pipes. In that case the first process in the
sequence is the leader of the process group. If a job runs in the
foreground, then its process group is bound to the controlling
terminal.  If a process group is bound to the controlling terminal,
then it can read keyboard input and generate output
to the terminal.   

The spawn\_job routine provided with the code shows how to use fork
and gets you started with some other logistics involving process
groups (described a bit below).  You're responsible for actually
calling spawn\_job from main and you will need to expand it
considerably.

## Exec

The child process resulting from a fork is a clone of the parent
process.  In particular, the child process runs the parent program
(e.g., dsh), initially with all of its active data structures as they
appeared in the parent at the time of the fork.  The parent and child
each have a (logical) copy of these data structures.  They may change
their copies indendently: neither will see the other's changes.

The exec() family of system calls (e.g., execve) enables the calling
process to execute an external program that resides in an executable
file.  An exec() system call never returns.  Instead, it transfers
control to the main procedure of the named program, running within the
calling process.  All data structures and other state relating to the
previous program running in the process---the calling program---are
destroyed.  There are a variety of different versions of exec designed
to be convenient to call depending on the circumstances - the one
you'll want to use is execvp.

## Wait

The wait() family of system calls (e.g., waitpid) allows a parent
process to query the status of a child process or wait for a child
process to change state.  In this lab, the main thing you'll want to
do is wait for all processes invoked by the shell (maybe with pipes)
to complete.

waitpid is the one you'll want to use to allow the parent to wait for
the children that are created. Note that you'll want to create all the
children in a piped command before beginning waiting for any of them.

## Input/Output redirection

Instead of reading and writing from stdin and stdout, one can choose
to read and write from a file.  The shell supports I/O redirection
using the special characters "\<" for input and "\>" for output
respectively.

Redirection is relatively easy to implement: just use close() on
stdout/stdin and then open() on a file.

With file descriptor, you can perform read and write to a file using
creat(), open() , read(), and write() system calls.

You may use the dup2() system call that duplicates an open file
descriptor onto the named file descriptor. For example, to redirect
all the standard error output stderr (2) to a standard output stdout
(1)}, simply invoke dup2()} as follows:

    /* close(stdout) and redirect stderr to stdout */
    dup2(2, 1);
    

## Pipelines

A pipeline is a sequence of processes chained by their standard
streams, so that the output of each process (stdout) feeds directly as
input (stdin) to the next one. If an command line contains a symbol
\|, the processes are chained by a pipeline.

The example below shows the contents of the file, produced by the
output of a cat command, are fed directly as an input to the to wc
command, which then produces the output to stdout.

    dsh$ /bin/cat inFile
    this is an input file
    dsh$ /bin/cat inFile | /bin/wc #Asynchronously starts two process: cat and wc; pipes the output of cat to wc
    1    5    22	


Pipes can be implemented using the pipe() and dup2() system calls. A
more generic pipeline can be of the form:

    p1 < inFile | p2 | p3 | .... | pn > outFile

where inFile and outFile are input and output files for redirection. 

The descriptors in the child are often duplicated onto standard input
or output. The child can then exec() another program, which inherits
the standard streams. dup2() is useful to duplicate the child
descriptors to stdin/stdout. For example, consider:

  
    int fd[2];
    pid_t pid;
    pipe(fd);
      
    switch (pid = fork()) {
	  case 0: /* child */
	    dup2(fd[0], STDIN_FILENO); /* close stdin (0); duplicate input end of the pipe to stdin */
	    execvp(...);
	  ....
    }


where dup2() closes stdin and duplicates the input end of the pipe to
stdin. The call to exec() will overlay the child's text segment (code)
with new executable and inherits standard streams from its
parent--which actually inherits the input end of the pipe as its
standard input! Now, anything that the original parent process sends
to the pipe, goes into the newly exec'ed child process.

## Handling terminal I/O and signals correctly

When a shell starts a child (or children in the case of a pipeline),
all those children should be placed in a process group.  That process
group should register as the one currently running in the tty.  This
allows a ctrl-C to kill processes in the pipe but not kill the shell
itself.

The given code in spawn\_job handles this for you with a few helper
functions (new\_child, set\_child\_pgid, and seize\_tty).  You
shouldn't need to change that code, but if you're curious as to what
the code does that is what it's for.

None of the process group behavior is actually tested by the automated
tests, so if you remove it (or mess it up) it shouldn't affect your
ability to get credit.  However, it's convenient to have it working as
you test your shell.

# EXTENDING
## Interactive bash scripter (40)
An interactive command-line tool that allows users to enter and execute bash scripts. Besides the built-in Unix functionalities, we will allow the shell to execute for loop, while loop, break, continue, variable declaration, and a limited amount of function calls. Also, each interactive scripter should have an independent environment (including user-defined environment variables). 
## Foreground & Background Management (20)
We allow the jobs to run in either foreground or background. We enable commands like “fg” and “bg”.
## History (20):
Back tracing the commands that have been executed so far and corresponding results.
## Test Program (20):
Test programs that verify the correctness of the scripter’s functionalities, foreground/background processes, and history logging, with a fair level of coverage.





