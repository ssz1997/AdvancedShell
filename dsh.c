#include "dsh.h"
// A dsh supports basic shell features: it spawns child processes, directs
// its children to execute external programs named in commands, passes arguments
// into programs, redirects standard input/output for child processes, chains
// processes using pipes, and monitors the progress of its children.
void seize_tty(pid_t callingprocess_pgid); /* Grab control of the terminal for the calling process pgid.  */
void continue_job(job_t *j);               /* resume a stopped job */
void spawn_job(job_t *j, bool fg);         /* spawn a new job */

// Implement spawn_job(), main(), and builtin_cmd()

/* Sets the process group id for a given job and process */
int set_child_pgid(job_t *j, process_t *p)
{
  if (j->pgid < 0) /* first child: use its pid for job pgid */
    j->pgid = p->pid;
  return (setpgid(p->pid, j->pgid));
}

/* Creates the context for a new child by setting the pid, pgid and tcsetpgrp */
void new_child(job_t *j, process_t *p, bool fg)
{
  /* establish a new process group, and put the child in
          * foreground if requested
          */

  /* Put the process into the process group and give the process
          * group the terminal, if appropriate.  This has to be done both by
          * the dsh and in the individual child processes because of
          * potential race conditions.  
          * */

  p->pid = getpid();

  /* also establish child process group in child to avoid race (if parent has not done it yet). */
  set_child_pgid(j, p);

  if (fg)               // if fg is set
    seize_tty(j->pgid); // assign the terminal

  /* Set the handling for job control signals back to the default. */
  signal(SIGTTOU, SIG_DFL);
}

/* Spawning a process with job control. fg is true if the 
 * newly-created process is to be placed in the foreground. 
 * (This implicitly puts the calling process in the background, 
 * so watch out for tty I/O after doing this.) pgid is -1 to 
 * create a new job, in which case the returned pid is also the 
 * pgid of the new job.  Else pgid specifies an existing job's 
 * pgid: this feature is used to start the second or 
 * subsequent processes in a pipeline.
 * 
 * A pipeline is a sequence of processes chained by their standard
 * streams, so that the output of each process (stdout) feeds directly as
 * input (stdin) to the next one. If an command line contains a symbol
 *, the processes are chained by a pipeline.
 * */

void spawn_job(job_t *j, bool fg)
{

  pid_t pid;
  process_t *p;
  int current_read = dup(STDIN_FILENO);
  int current_write = -1;
  int next_read = -1;

  // assert(current_read != -1);

  for (p = j->first_process; p; p = p->next)
  {

    /* YOUR CODE HERE? */
    /* Builtin commands are already taken care earlier */
    // pipe[1] write, pipe[0] read
    if (p->next == NULL)
    {
      current_write = dup(STDOUT_FILENO);
      next_read = -1;
    }
    else
    {
      int cur_pipe[2];
      pipe(cur_pipe);
      current_write = cur_pipe[1];
      next_read = cur_pipe[0];
    }

    switch (pid = fork())
    {

    case -1: /* fork failure */
      perror("fork");
      exit(EXIT_FAILURE);

    case 0: /* child process  */
      p->pid = getpid();
      new_child(j, p, fg);

      /* YOUR CODE HERE?  Child-side code for new process. */
      /*  char *ifile;                 stores input file name when < is issued */
      /*  char *ofile;                 stores output file name when > is issued */
      // References https://linux.die.net/man/3/open
      // dup2(oldfd, newfd)
      if (p->ofile)
      {
        int pfd = open(p->ofile, O_RDWR | O_CREAT);
        close(current_write);
        current_write = pfd;
      }

      if (p->ifile)
      {
        int pfd = open(p->ifile, O_RDWR | O_CREAT);
        close(current_read);
        current_read = pfd;
      }
      // execvp("cmd", arg[])

      // DUP curr_read to STDIN_FILENO
      dup2(current_read, STDIN_FILENO);
      dup2(current_write, STDOUT_FILENO);
      close(current_read);
      close(current_write);

      execvp(p->argv[0], p->argv);
      perror("New child should have done an exec");
      exit(EXIT_FAILURE); /* NOT REACHED */
      break;              /* NOT REACHED */

    default: /* parent */
      /* establish child process group */
      p->pid = pid;
      set_child_pgid(j, p);
      close(current_read);
      close(current_write);

      /* YOUR CODE HERE?  Parent-side code for new process.  */
      current_read = next_read;
    }

    /* YOUR CODE HERE?  Parent-side code for new job.*/
  }

  for (p = j->first_process; p; p = p->next)
  {
    waitpid(p->pid, NULL, 0);
  }

  seize_tty(getpid()); // assign the terminal back to dsh
}

/* Sends SIGCONT signal to wake up the blocked job */
void continue_job(job_t *j)
{
  if (kill(-j->pgid, SIGCONT) < 0)
    perror("kill(SIGCONT)");
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 * it immediately.  
 */
bool builtin_cmd(job_t *last_job, int argc, char **argv)
{

  /* check whether the cmd is a built in command
  */
  if (!strcmp(argv[0], "quit"))
  {
    /* Your code here */
    exit(EXIT_SUCCESS);
  }
  else if (!strcmp("cd", argv[0]))
  {
    if (chdir(argv[1]) < 0)
    {
      perror("cd");
    }
    return true;
  }
  return false; /* not a builtin command */
}

/* Build prompt messaage */
char *promptmsg()
{
  /* Modify this to include pid */
  return "dsh$ ";
}

int main()
{

  init_dsh();
  // DEBUG("Successfully initialized\n");

  while (1)
  {
    job_t *j = NULL;
    if (!(j = readcmdline(promptmsg())))
    {
      if (feof(stdin))
      { /* End of file (ctrl-d) */
        fflush(stdout);
        printf("\n");
        exit(EXIT_SUCCESS);
      }
      continue; /* NOOP; user entered return or spaces with return */
    }

    /* Only for debugging purposes to show parser output; turn off in the
         * final code */
    // if (PRINT_INFO)
    //   print_job(j);

    /* Your code goes here */
    /* You need to loop through jobs list since a command line can contain ;*/
    /* Check for built-in commands */
    /* If not built-in */
    /* If job j runs in foreground */
    /* spawn_job(j,true) */
    /* else */
    /* spawn_job(j,false) */
    while (j != NULL)
    {
      process_t *p = j->first_process;
      // builtin_cmd(job_t *last_job, int argc, char **argv)
      if(!builtin_cmd(NULL, p->argc, p->argv)) {
        spawn_job(j, true);
      }
      j = j->next;
    }
    
  }
}
