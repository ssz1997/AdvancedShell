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
## Interactive bash scripter
Next, we created an interactive bash scripter that allows users to execute interactive computing tasks such as function declaration using variables, for loop, and some related functions. All declared functions and variables should be preserved throughout the session unless specified by users otherwise. Bash scripter can help system administrators automate things by using a loop to execute repetitive tasks. On a large system, this would save tons of time.
In order to create a shell environment, we will first try to catch the command info and see if the command is “shell”, and once the shell command has been called, we will first create a while loop that keeps reading the new line from the stdin until “exit” has been called.
To perform the variable declaration, we create a hashmap to store the variable and its assigned value, and once the incoming command contains the ‘=’ sign, we will separate the input string and put the corresponding key-value pair into the unordered map for further operations.
One function we realized could be helpful is doing mathematical operations, so we decide to add a simple arithmetic function into our shell, in which if the normal addition or subtraction is typed into the shell, the user can get the result immediately, and the user can also perform assignment operations with it. For example, we can do “var=(1+2)-3”, and when we try to echo $var, the output would be 0 because the value has been calculated directly. 
In this function, we use some algorithm to first parse the incoming command line and transfer any variable ($var) into the value it has, and then we perform the calculation by using stack and flag. Therefore, we are able to perform a long string of calculations such as (4-3) + 1 + 2, etc.
Another interesting function for a shell to have is to allow the variable to contain the result of their Unix operation. For example, if we perform var = $(pwd), then var would contain the result of “pwd” such as “/home/project”, and after that when we perform echo $var, “/home/project” would be printed into the shell without actually performing “pwd” again.
Specifically, in this function, we would first check whether the value is a Unix operation, and we would process the command and pass them into the original spawn_job function. However, when we declare a variable, we don’t really want the operation to print out their output into the terminal/shell, so we decided to use a flag to clarify the assignment operation and save the output into our local hashmap. Specifically, since the STDOUT_FILENO is essentially a file descriptor, we would first use dup2 to switch the file descriptor into a local log file, and then the spawn_job finish, we would first read the output from the log file and assign the output as the value and create a new key-value pair and store them.
One of the most important functions is for loop. In our program, we would use strict bash syntax( for i in {1..4}) to perform for loop operation, we would first check whether the command line contains the for-loop’s keyword, and once we assure this is a for loop, we would first extract the start, end, and step number from the command line. and we would keep getting a new line from the coming command lines and store all the operations into a vector, and once the keyword “done” appears, we would break the getline’s while loop and perform for loop operations by traversing the commands vector using the start, end, and step numbers.
By using the for loop, we can perform Unix operations repetitively and storing the variable declarations’ results, and it can make our interactive shell extremely useful. Specifically, we can first type “for i in {1..5}”, and then type “do”, and “echo 1”, and “done”, and the shell would print 1 five times, and so does any other operation.
## History
Furthermore, we created history/logging functionality. The user is then able to trace back what commands have been executed and what are the results, including stdout, stderr, and the output redirection. Therefore, instead of having to scroll back and forth to find the output of a particular command, now the user is able to easily find the index of that command and then find its corresponding output. By calling “history”, we show the most recent 100 commands that were run. And by calling “history i”, where i is the index of the command that is indicated in the “history” call. A background job is different from a typical Unix shell. The output is not directly printed in the terminal but will be directed to its corresponding and unique file. We think it kind of makes sense that the output of the background job does not out and interfere with the foreground job output if there is any. 
After the command is parsed and the job is created, the command is stored in the memory. The array has a size of 100 and thus we can store the most recent 100 commands. By calling “history”, we iterate through the array and print saved commands to the console with their id starting from 1. 
To store the output of a command, we need to consider foreground jobs and background jobs separately. For foreground jobs, similar to what we do in the interactive bash shell, instead of outputting the results to the screen, we call dup2 and direct the output to a file. When the process is finished, function parent_wait would return and then look at the file, identified by its process number, and then print it to the screen, and at the same time storing what it prints to our log file called output.log, where all the outputs are stored. In other words, we add another layer between the process and the terminal, in which we intercept the output, store it, and then print it out. We delete the file after we are done reading it, so we won’t use too much memory. 
For background jobs, we wanted to print everything to the console, like a real Unix shell. However, if we want to at the same time store the outputs, we either directly read from the console, which is not practical, or save it to some other files, like what we do for the foreground jobs. But to output them to the console, we need to know when there is stuff or new stuff, we could print, and for a background job we have no idea when it stops or generates any output, unlike foreground jobs. Therefore, the only way is to not actually print the outputs to the shell, and that actually makes some sense, as we said previously, the output of a background job should not interfere with other stuff. The reason we want to run something in the background is that we want to run other stuff in the foreground and if their outputs are mixed together and we cannot figure out which is which, it could bring issues. Therefore, we call dup2 to redirect the output into its corresponding and unique file identified by the process id and let the output file, output.log, write down where the output is. When the user wants to look at the output, the output file would let him/her know where to find it.
Now as we have all the outputs of the jobs, by calling “history i”, where i is the index of the command indicated in the command history array






