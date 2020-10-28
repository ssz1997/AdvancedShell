# Implementing a Shell

Unix embraces the philosophy:

    Write programs that do one thing and do it well.
    Write programs to work together.
    
The idea of a "shell" command interpreter is central to the Unix programming
environment.  As a command interpreter, a shell executes commands that you
enter in response to its prompt in a terminal window, which is called the
*controlling terminal*. The inputs to the shell are program executable or simply commands,
and when you type in the input (in response to its prompt), the shell spawns 
a child process that executes that command along with any arguments passed for that command.
The shell also links the input/output from terminal to other commands in the pipeline or to standard
files. Thus you can also use a shell as a scripting language that combines subprograms to perform more
complex functions.

# The Devil Shell

For this lab you use Unix system calls to implement a basic shell.
We call it a *devil shell*, abbreviated as dsh.

A dsh supports basic shell features: it spawns child processes, directs
its children to execute external programs named in commands, passes arguments
into programs, redirects standard input/output for child processes, chains
processes using pipes, and monitors the progress of its children.  

The dsh also groups processes for job control.   For our purposes, a
job is a group of one or more commands that the shell executes together
and treats as a unit.   When the shell is being used interactively (i.e.,
there is a controlling terminal), then at most one job is in the
``foreground'' at any given time.  If a job is in the foreground then
any input on the controlling terminal is directed to the foreground job,
rather than to the shell itself, and the shell waits for the foreground job to
quit (exit).

Dsh avoids the use of Unix signals.  The Unix signal mechanism
was an early abstraction for event handling.  It was botched in its
initial conception and then reworked in piecemeal fashion over
decades, resulting in multiple versions and incompatibilities.
Signals are central to Unix, but we are glossing over them in this
course.  If you find yourself trying to understand signals, then
please talk to us first.  We recommend the CS:APP book if you want to
know how real shells use signals.

Although we try to ignore them (literally), signals are intricately
wound into the user experience of any shell.  If there is a foreground
job, you can cause it to exit by typing ctrl-c on the controlling
terminal.  The default behavior for these signals is to kill or stop
the receiving process.  If there is no foreground job, then the dsh
itself has control of the terminal: the signals are directed to dsh
which may cause it to exit or stop.

The shell itself also exits if it reads an *end-of-file* (EOF) on
its input.  The shell reads an EOF when it reaches the end of a
command script, or if you type ctrl-d to it on the controlling
terminal.  EOF is not a signal, but just a marker for the end of a
file or stream.

# Inputs

The shell reads command lines from its standard input and interprets
them.  It continues until it is killed or there are no more commands
(EOF).  We provide a command line parser to save you the work of
writing one yourself, or at least to give an example of how to do it.

The shell prints a prompt of your choosing (e.g., dsh-277, where 277
is the shell process id) before reading each input line.  We've set it
up to automatically disable the shell print in non-interactive mode, so
you don't need to worry about it for our tests.


If an input line starts with the special character #, then the line is
a comment: the entire line is ignored.  Empty lines are also ignored.
Any other line is a *command line*.  If a command line contains a #
character, then the remainder of the line is a comment and is ignored:
the # and any succeeding characters are not part of the
command line.

Upon receiving a command line, the shell interprets the line as a
sequence of one or more commands, separated by the special characters
";" or "\|".  If a command is followed by a ";" then the shell
completes its processing of the command before moving to the next
command in the sequence (the successor), if there is one.  The special
character \| indicates a pipeline: if a command is followed by a \|
then the shell arranges for its standard output (stdout) to pass
through a pipe to the standard input (stdin) of its successor.  The
command and its successor are grouped in the same job.  If there is no
successor then the command is malformed.

Each command is a sequence of substrings (tokens) separated by blank space.
The first token names the command: it is either the name of a built-in command
or the name of an external program (an executable file) to run.  The built-in
commands are discussed later in the handout.

The remaining tokens in a command specify arguments for the command.
dsh has limited support for input/output redirection:

  - If an argument begins with the special character \< then the shell
    arranges for the command to read its stdin from a file: the rest
    of that argument is the name of the input file.

  - If an argument begins with the special character \> then the shell
    arranges for the command to write its stdout to a file: the rest
    of that argument is the name of the output file.

All other arguments for a command are argument strings. The shell
passes the argument strings to the command in the order in which they
appear on the command line.  The command interprets its argument
strings in a command-specific fashion.  External programs receive
these arguments through an array of strings called the argv array.
The argv array has argc elements, where argc \> 1.  The first string
in argv is the name of the command.  The remaining strings in argv are
the argument strings as they appear on the command line.   

Note that all of the parsing features are implemented for you
already - for example, the parser breaks a line up into a series of
processes and a series of jobs.  The parser also detects input/output
redirect characters and puts them as ifile and ofile in the job
structure, etc.  Your job will be to make running the processes and
jobs work correctly.

# Built-in commands

The shell executes built-in commands in its own context, without
spawning a child process or a job.  This means that a line like
"doThing" applied to a shell has two possible meanings - it might be a
built in command, or it might be an attempt to execute a executable
called doThing. Shells check for their built-in commands first, then
if there are no matches they assume the user was attempting to invoke
an executable.

Builtins do things that a separate program can't do - For example, the
command quit terminates a shell.

The only other builtin command in dsh is "cd" which changes the
current directory of the shell.  Real shells have many other built in
commands dealing with setting special variables, moving jobs between
the foreground and the background, etc.


We've implemented all the built-ins for you, but you will have to call
the function (builtin\_cmd) from your shell's main.  If that function
returns false, it means the given command doesn't match a built-in and
should be considered an executable.


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

# Getting started

You should be able to build dsh by typing make with the included
Makefile.  Note that all your changes will be in dsh.c.  The parsing
of commands is implemented for you in parser.c - don't make changes to
the parser.

## Suggested plan for implementation

 1. Read this handout. Read the material from OSTEP \cite{OSTEP-cpu} on process creation and execution.  Watch the videos about unix.
 2. Read the man pages for fork(), exec(), wait(), dup2(), open(), read(), write(), and exit().
 2. Look at the main and in dsh.h - understand the process and job structs that are returned by the parser.
 3. Using the parser we gave, start writing single commands.  Test with parameters (e.g. "ls -l -h").
 4. Add support for running multiple commands seperated by ";".  This is much easier than pipe, because these processes are completely calls to spawn child and have no special IO redirection.
 5. Add input and output redirection.
 6. Add support for pipes using the pipe() and dup2() system call.  This is by far the trickiest part.
 7. Make sure the tests run locally
 8. You're done!

# Running the tests

Run the tests like this:

    bash ./tests.bash

Note that tests compare your output to an existing shell (the classic
unix sh).  This means that any extraneous prints will cause the tests
to fail.  Output on stderr is ignored however, so DEBUG statements
are fine (and you can pass them parameters like printf).

