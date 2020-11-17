#include "dsh.h"
#include <stdarg.h>
#include <unordered_map>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

using namespace std;
static char prompt_head[20];

unordered_map<string, int> intVariables;
unordered_map<string, string> strVariables;

job_t *job_list = NULL; // first job
bool assigncmd = false;

static const int PIPE_READ = 0;
static const int PIPE_WRITE = 1;

// helper functions
job_t *search_job(int jid);
job_t *search_job_pos(int pos);
void addJob(job_t *j);                                    // add job to job list
process_t *getProcess(int pid);                           // get the process by process id
void redirect(process_t *p);                              // redirect the input/output
int set_pgid(job_t *j, process_t *p);                     // set pgid for a job
void new_child(job_t *j, process_t *p, bool fg);          // create context for new child process
void continue_job(job_t *j);                              // continue a stopped job
char *promptmsg();                                        // heading
void parent_wait(job_t *j, int fg);                       // parent wait for child to finish
void print_jobs();                                        // print jobs in the list
bool builtin_cmd(job_t *last_job, int argc, char **argv); // execute built-in cmd
void spawn_job(job_t *j, bool fg);                        // spawn a new job

void assignment(string cmdline);
int *getForLoop(string cmdline);

job_t *search_job(int jid)
{
    DEBUG("search_job");
    job_t *job = job_list;
    while (job != NULL)
    {
        if (job->pgid == jid)
            return job;
        job = job->next;
    }
    return NULL;
}

job_t *search_job_pos(int pos)
{
    DEBUG("search_job_pos");
    job_t *job = job_list;
    int count = pos;
    while (job != NULL)
    {
        if (count == 1)
        {
            return job;
        }
        if (job->next == NULL)
        {
            return job;
        }
        count--;
        job = job->next;
    }
    return NULL;
}

void addJob(job_t *j)
{
    if (j)
    {
        if (job_list == NULL)
        {
            job_list = j;
        }
        else
        {
            job_t *cur = job_list;
            while (cur->next != NULL)
            {
                cur = cur->next;
            }
            cur->next = j;
        }
    }
}

process_t *getProcess(int pid)
{
    job_t *cur = job_list;
    while (cur)
    {
        process_t *p = cur->first_process;
        while (p)
        {
            if (p->pid == pid)
            {
                return p;
            }
            p = p->next;
        }
        cur = cur->next;
    }
    return NULL;
}

void redirect(process_t *p)
{
    if (p->ifile)
    {
        int fd = open(p->ifile, O_RDONLY);
        if (fd >= 0)
        {
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
    }

    if (p->ofile)
    {
        int fd = creat(p->ofile, 0644);
        if (fd >= 0)
        {
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
    }
}

int set_pgid(job_t *j, process_t *p)
{
    j->pgid = j->pgid < 0 ? p->pid : j->pgid;
    return setpgid(p->pid, j->pgid);
}

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
    set_pgid(j, p);

    if (fg)                 // if fg is set
        seize_tty(j->pgid); // assign the terminal

    /* Set the handling for job control signals back to the default. */
    signal(SIGINT, SIG_DFL);
}

void continue_job(job_t *j)
{
    process_t *main_process = getProcess(j->pgid);
    process_t *p = main_process;
    while (p)
    {
        p->stopped = false;
        p = p->next;
    }

    if (kill(-j->pgid, SIGCONT) < 0)
    {
        printf("Kill SIGCONT");
    }
    if (isatty(STDIN_FILENO))
    {
        seize_tty(getpid());
    }
}

char *promptmsg()
{
    /* Modify this to include pid */
    sprintf(prompt_head, "dsh-%d$ ", (int)getpid());
    return prompt_head;
}

void parent_wait(job_t *j, int fg)
{
    DEBUG("parent_wait");
    if (fg)
    {
        int status, pid;
        while ((pid = waitpid(WAIT_ANY, &status, WUNTRACED)) > 0)
        {
            process_t *p = getProcess(pid);
            if (WIFEXITED(status))
            {
                p->completed = true;
                if (status == EXIT_SUCCESS)
                {
                    printf("%d (Completed): %s\n", pid, p->argv[0]);
                }
                else
                {
                    printf("%d (Failed): %s\n", pid, p->argv[0]);
                }
                fflush(stdout);
            }
            else if (WIFSTOPPED(status))
            {
                DEBUG("Process %d stopped", p->pid);
                if (kill(-j->pgid, SIGSTOP) < 0)
                {
                    //                    logger(STDERR_FILENO,"Kill (SIGSTOP) failed.");
                    printf("Kill SIGSTOP failed");
                }
                p->stopped = true;
                j->notified = true;
                j->bg = true;
                print_jobs();
            }

            else if (WIFCONTINUED(status))
            {
                DEBUG("Process %d resumed", p->pid);
                p->stopped = 0;
            }
            else if (WIFSIGNALED(status))
            {
                DEBUG("Process %d terminated", p->pid);
                p->completed = 1;
            }
            if (job_is_stopped(j) && isatty(STDIN_FILENO))
            {
                seize_tty(getpid());
                break;
            }
        }
    }
}

void print_jobs()
{
    int count = 1;
    //    remove_zombies();
    job_t *j = job_list;
    if (j == NULL)
    {
        //        char *msg = "No jobs are running\n";
        //        write(STDOUT_FILENO, msg, strlen(msg));
        return;
    }
    while (j != NULL)
    {
        printf("[%d]", count);
        if (j->notified)
            printf("    Stopped     ");
        else
        {
            if (j->bg)
                printf(" bg ");
            else
                printf(" fg ");
            printf(" Running        ");
        }
        printf("%s\n", j->commandinfo);
        j = j->next;
        count++;
    }
    fflush(stdout);
}

bool builtin_cmd(job_t *last_job, int argc, char **argv)
{

    /* check whether the cmd is a built in command
     */
    DEBUG("builtin_cmd");

    if (!strcmp(argv[0], "quit"))
    {
        exit(EXIT_SUCCESS);
    }
    else if (!strcmp("jobs", argv[0]))
    {
        print_jobs();
        return true;
    }
    else if (!strcmp("cd", argv[0]))
    {
        if (argc <= 1 || chdir(argv[1]) == -1)
        {
            //            logger(STDERR_FILENO,"Error: invalid arguments for directory change");
        }
        return true;
    }

    //Background command, works as long as next argument is a reasonable id
    else if (!strcmp("bg", argv[0]))
    {
        int position = 0;
        job_t *job;
        if (argc != 2 || !(position = atoi(argv[1])))
        {
            printf("%d %d", position, argc);
            //            logger(STDERR_FILENO,"Error: invalid arguments for bg command");
            return true;
        }
        if (!(job = search_job_pos(position)))
        {
            printf("%d %d", position, argc);
            //            logger(STDERR_FILENO, "Error: Could not find requested job");
            return true;
        }
        if (job_is_completed(job))
        {
            //            logger(STDERR_FILENO, "Error: job is already completed!");
            return true;
        }

        printf("#Sending job '%s' to background\n", job->commandinfo);
        fflush(stdout);
        continue_job(job);
        job->bg = true;
        job->notified = false;
        return true;
    }

    //Foreground command, works as long as next argument is a reasonable id
    else if (!strcmp("fg", argv[0]))
    {
        int pos = 0;
        job_t *job;

        //no arguments specified, use last job
        if (argc == 1)
        {
            job = search_job_pos(-1);
        }
        //right arguments given, find respective job
        else if (argc == 2 && (pos = atoi(argv[1])))
        {
            if (!(job = search_job_pos(pos)))
            {
                //                logger(STDERR_FILENO, "Could not find requested job");
                return true;
            }
            if (job->notified == false)
            {
                //                logger(STDERR_FILENO, "The job is already in foreground.");
                return true;
            }
            if (job_is_completed(job))
            {
                //                logger(STDERR_FILENO,"Job already completed!");
                return true;
            }
        }
        else
        {
            //            logger(STDERR_FILENO,"Invalid arguments for fg command");
            return true;
        }

        printf("#Bringing job '%s' to foreground\n", job->commandinfo);
        fflush(stdout);
        continue_job(job);
        job->bg = false;
        if (isatty(STDIN_FILENO))
            seize_tty(job->pgid);
        parent_wait(job, true);
        return true;
    }
    return false; /* not a builtin command */
}

void spawn_job(job_t *j, bool fg)
{
    pid_t pid;
    process_t *p;
    job_list = NULL;
    addJob(j);
    int prev_pipe[2];

    for (p = j->first_process; p; p = p->next)
    {

        /* YOUR CODE HERE? */
        if (p->argv[0] == NULL)
        {
            continue;
        }
        int next_pipe[2];

        pipe(next_pipe);

        /* Builtin commands are already taken care earlier */
        switch (pid = fork())
        {

        case -1: /* fork failure */
            perror("fork");
            exit(EXIT_FAILURE);

        case 0: /* child process  */
            p->pid = getpid();

            set_pgid(j, p);

            if (p != j->first_process)
            {
                close(prev_pipe[PIPE_WRITE]);
                dup2(prev_pipe[PIPE_READ], STDIN_FILENO);
                close(prev_pipe[PIPE_READ]);
            }
            if (p->next)
            {
                close(next_pipe[PIPE_READ]);
                dup2(next_pipe[PIPE_WRITE], STDOUT_FILENO);
                close(next_pipe[PIPE_WRITE]);
            }
            else
            {
                dup2(STDOUT_FILENO, next_pipe[PIPE_WRITE]);
                // redirect to local
                if (assigncmd)
                {
                    int log = open("slogs.log", 0666);
                    dup2(log, STDOUT_FILENO);
                    close(log);
                }
                close(next_pipe[PIPE_READ]);
                close(next_pipe[PIPE_WRITE]);
            }

            new_child(j, p, fg);
            redirect(p);
            if (execvp(p->argv[0], p->argv) < 0)
            {
                exit(EXIT_FAILURE);
            }

            perror("New child should have done an exec");
            exit(EXIT_FAILURE); /* NOT REACHED */

        default: /* parent */
            /* establish child process group */
            p->pid = pid;
            set_pgid(j, p);

            /* YOUR CODE HERE?  Parent-side code for new process.  */
            prev_pipe[PIPE_WRITE] = next_pipe[PIPE_WRITE];
            prev_pipe[PIPE_READ] = next_pipe[PIPE_READ];
            break;
        }

        /* YOUR CODE HERE?  Parent-side code for new job.*/
        if (p->next == NULL)
        {
            close(next_pipe[PIPE_READ]);
        }
        close(prev_pipe[PIPE_WRITE]);

        parent_wait(j, fg);
    }
}

void assignment(string cmdline)
{
    string var = cmdline.substr(0, cmdline.find("="));
    string value = cmdline.substr(cmdline.find("=") + 1);
    int index = 0;
    if ((index = value.find('$')) != string::npos)
    {
        if (value[index + 1] == '(')
        {
            // its a command
            string unixcmd = value.substr(index + 2, value.find(')') - index - 2);
            assigncmd = true;
            job_t *tmp = readcommandline(unixcmd.c_str());
            while (tmp)
            {
                int argc = tmp->first_process->argc;
                char **argv = tmp->first_process->argv;
                if (!builtin_cmd(tmp, argc, argv))
                {
                    spawn_job(tmp, !(tmp->bg));
                }
                tmp = tmp->next;
            }

            FILE *out = fopen("slogs.log", "r+");
            fseek(out, 0, SEEK_END);
            int lSize = ftell(out);
            rewind(out);
            // printf("%d\n", lSize);
            if (lSize != 0)
            {
                char *buffer = (char *)calloc(1, lSize + 1);
                if (1 != fread(buffer, lSize, 1, out))
                {
                    fclose(out), free(buffer), fputs("1entire read fails", stderr), exit(1);
                }
                string cmdOutput(buffer);
                strVariables[var] = cmdOutput;
            }
            assigncmd = false;
        }
        else
        {
            strVariables[var] = strVariables.count(value) ? strVariables[value.substr(1)] : "";
        }
    }

    // if (value.find_first_not_of("0123456789") == string::npos)
    // {
    //     int tmp = stoi(value);
    //     intVariables[var] = tmp;
    // }
    // else
    else if (value.find('"') != string::npos)
    {
        strVariables[var] = value.substr(1, value.length()-2);
    }
}

int *getForLoop(string cmdline)
{
    // cout << cmdline << endl;
    int *arr = new int[3];
    arr[2] = 1;

    string range = cmdline.substr(cmdline.find("{"));
    size_t comma = std::count(range.begin(), range.end(), '.');
    // cout << comma << endl;
    // cout << range << endl;

    if (comma == 2)
    {
        arr[0] = stoi(range.substr(1, range.find(".") - 1));
        int tmp = range.find(".") + 2;
        arr[1] = stoi(range.substr(tmp, range.find("}") - tmp));
    }
    else
    {
        arr[0] = stoi(range.substr(1, range.find(".") - 1));
        range = range.substr(range.find(".") + 2);
        arr[1] = stoi(range.substr(0, range.find(".")));
        range = range.substr(range.find(".") + 2);
        arr[2] = stoi(range.substr(0, range.find("}")));
    }
    // cout << arr[0] << arr[1] << endl;
    return arr;
}

string parse(string cmdline)
{
    string res = "";
    for (int i = 0; i < cmdline.size(); i++) {
        if (cmdline[i] == '$') {
            int tp = i;
            while (cmdline[tp] != ' ' && tp < cmdline.size()) {
                tp++;
            }
            string key = cmdline.substr(i + 1, tp - i -1);
            res += strVariables.count(key) ? strVariables[key] : "";
        } else {
            res.push_back(cmdline[i]);
        }
    }
    return res;
}

int main()
{
    init_dsh();
    //    DEBUG("Successfully initialized\n");

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

        if (strcmp(j->commandinfo, "shell") == 0)
        {
            while (1)
            {
                string cmdline;

                fprintf(stdout, ">>> ");
                getline(cin, cmdline);
                if (cmdline.compare("exit") == 0)
                {
                    break;
                }
                else if (cmdline.find("=") != string::npos)
                {
                    assignment(cmdline);
                }
                // for i in range(start, end, step):
                else if (cmdline.substr(0, 3).compare("for") == 0)
                {
                    string var = cmdline.substr(cmdline.find(" ") + 1);
                    // cout << "var: " << var << endl;
                    var = var.substr(0, var.find(" "));
                    // cout << "var: " << var << endl;
                    int *forinfo = getForLoop(cmdline);

                    int start = forinfo[0];
                    int end = forinfo[1];
                    int step = forinfo[2];
                    delete forinfo;
                    intVariables[var] = start;

                    string forcommand;
                    vector<string> commands;
                    while (getline(cin, forcommand))
                    {
                        forcommand = forcommand.substr(forcommand.find_first_not_of(" \t"));
                        if (forcommand == "done")
                        {
                            break;
                        }
                        else
                        {
                            commands.push_back(forcommand);
                        }
                    }
                    for (intVariables[var] = start; intVariables[var] <= end; intVariables[var] += step)
                    {
                        for (int i = 1; i < commands.size(); i++)
                        {
                            string tcmd = commands[i];
                            if (tcmd.find("=") != string::npos)
                            {
                                assignment(tcmd);
                            }
                            else
                            {
                                job_t *tmp = readcommandline(tcmd.c_str());
                                while (tmp)
                                {
                                    int argc = tmp->first_process->argc;
                                    char **argv = tmp->first_process->argv;
                                    if (!builtin_cmd(tmp, argc, argv))
                                    {
                                        spawn_job(tmp, !(tmp->bg));
                                    }
                                    tmp = tmp->next;
                                }
                            }
                        }
                    }

                    // use for loop to exe every command
                }
                else
                {   cmdline = parse(cmdline);
                    job_t *tmp = readcommandline(cmdline.c_str());
                    while (tmp)
                    {
                        int argc = tmp->first_process->argc;
                        char **argv = tmp->first_process->argv;
                        if (!builtin_cmd(tmp, argc, argv))
                        {
                            spawn_job(tmp, !(tmp->bg));
                        }
                        tmp = tmp->next;
                    }
                }
            }
            free(j->commandinfo);
            free(j);
            continue;
        }

        /* Only for debugging purposes to show parser output; turn off in the
         * final code */
        //        if(PRINT_INFO) print_job(j);

        /* Your code goes here */
        /* You need to loop through jobs list since a command line can contain ;*/
        /* Check for built-in commands */
        /* If not built-in */
        /* If job j runs in foreground */
        /* spawn_job(j,true) */
        /* else */
        /* spawn_job(j,false) */

        while (j)
        {
            int argc = j->first_process->argc;
            char **argv = j->first_process->argv;
            if (!builtin_cmd(j, argc, argv))
            {
                spawn_job(j, !(j->bg));
            }
            j = j->next;
        }
    }
}