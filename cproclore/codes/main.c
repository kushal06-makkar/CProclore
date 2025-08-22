#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include "hop.h"
#include "reveal.h"
#include "proclore.h"
#include "seek.h"
#include "echo.h"
#include "log.h"
#include "activities.h"
#include "neonate.h"
#include "alias.h"
#include "iman.h"
int l = 0;
pid_t foreground_pid = -1;
void sigint_handler(int signum)
{
    if (foreground_pid != -1)
    {
        kill(foreground_pid, SIGINT);
    }
}
void sigtstp_handler(int signum)
{
    if (foreground_pid != -1)
    {
        printf("\nStopping process %d\n", foreground_pid);
        kill(foreground_pid, SIGTSTP);
    }
}

void execute_background1(char *bg_command)
{
    if (bg_command && strlen(bg_command) > 0)
    {

        char *input_file = NULL;
        char *output_file = NULL;
        int append = 0;
        char *redir_in = strchr(bg_command, '<');
        char *redir_i = strchr(bg_command, '<');
        if (redir_in)
        {
            *redir_in = '\0';
            redir_in++;
            while (*redir_in && isspace(*redir_in))
            {
                redir_in++;
            }
            while (*redir_in && !isspace(*redir_in))
            {
                redir_in++;
            }
            while (*redir_in && isspace(*redir_in))
            {
                redir_in++;
            }
            strcat(bg_command, redir_in);
            char *check = strchr(bg_command, '<');
            if (check)
            {
                printf("Invalid command\n");
                return;
            }
        }

        if (redir_in)
        {
            input_file = strtok(redir_i + 1, " ");
        }
        pid_t pid = fork();
        if (pid == 0)
        {

            if (input_file)
            {
                int fd_in = open(input_file, O_RDONLY);
                if (fd_in < 0)
                {
                    perror("Error opening input file");
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            setpgid(0, 0);

            printf("[%d]\n", getpid());
            add_process(bg_command, getpid(), "Running");
            char *args[100];
            char *arg = strtok(bg_command, " ");
            int i = 0;
            while (arg != NULL)
            {
                args[i++] = arg;
                arg = strtok(NULL, " ");
            }
            args[i] = NULL;
            int c = execvp(args[0], args);
            if (c == -1)
            {
                remove_process(getpid());
            }
            if (c < 0)
            {
                printf("Background process ended abnormally with PID [%d]\n", getpid());
                exit(EXIT_FAILURE);
            }

            exit(EXIT_SUCCESS);
        }
        else if (pid > 0)
        {
            add_process(bg_command, pid, "Running");
        }
        else
        {
            perror("fork failed");
        }
    }
}
void execute_foreground1(char *fg_command, char *home, char **prevdir, char *currentdir, char log[LOG_SIZE][6000], int *log_count, int *last_cmd_time, char *last_cmd, int *flag, int *pf)
{
    if (fg_command && strlen(fg_command) > 0)
    {

        char *os = (char *)malloc(sizeof(char) * 10000);
        strcpy(os, fg_command);
        time_t start_time = time(NULL);
        char *redir_in = strchr(os, '<');
        if (redir_in)
        {
            if (strncmp(fg_command, "hop", 3) == 0 || strncmp(fg_command, "echo", 4) == 0 || strncmp(fg_command, "reveal", 6) == 0 || strncmp(fg_command, "seek", 4) == 0 || strncmp(fg_command, "proclore", 8) == 0 || strncmp(fg_command, "log execute", 11) == 0 || strncmp(fg_command, "ping", 4) == 0 || strncmp(fg_command, "fg", 2) == 0 || strncmp(fg_command, "bg", 2) == 0 || strncmp(fg_command, "neonate-n", 9) == 0)
            {

                if (redir_in)
                {
                    *redir_in = '\0';
                    redir_in++;
                    while (*redir_in && isspace(*redir_in))
                    {
                        redir_in++;
                    }
                    while (*redir_in && !isspace(*redir_in))
                    {
                        redir_in++;
                    }
                    while (*redir_in && isspace(*redir_in))
                    {
                        redir_in++;
                    }
                    strcat(os, redir_in);
                }

                char *check = strchr(os, '<');
                if (check)
                {
                    printf("Invalid command\n");
                    return;
                }
            }
        }

        if (strncmp(os, "hop", 3) == 0)
        {

            char *x = os + 3;
            while (*x == ' ')
                x++;
            hopcmd(x, home, *prevdir, NULL, pf);
            strcpy(*prevdir, currentdir);
        }
        else if (strncmp(os, "echo", 4) == 0)
        {
            char *echo_cmd = strstr(os, "echo");
            if (echo_cmd)
            {
                ecko(echo_cmd + 4, pf);
            }
        }
        else if (strncmp(os, "reveal", 6) == 0)
        {

            char *x1 = os + 6;
            char flags[10] = "";
            char *p = ".";
            int ind = 0;
            while (*x1 == ' ')
                x1++;
            while (*x1)
            {
                if (*x1 == '-' && (*(x1 + 1) == 'a' || *(x1 + 1) == 'l'))
                {
                    x1++;
                    while (*x1 && *x1 != ' ')
                        flags[ind++] = *x1++;
                }
                else
                {
                    p = x1;
                    break;
                }
                while (*x1 == ' ')
                    x1++;
            }
            flags[ind] = '\0';
            reveal(flags, p, home, *prevdir);
        }
        else if (strncmp(os, "seek", 4) == 0)
        {

            char *args[100];
            int arg_count = 0;
             args[arg_count++] = "seek";
            char *arg = strtok(os, " ");

            // Skip the "seek" part
           
            arg = strtok(NULL, " ");
            
            // Flags checking
            int has_flags = 0;
            int has_search_item = 0;
            int valid_flags = 1;

            while (arg != NULL)
            {
                if (arg[0] == '-') // Check for valid flags
                {
                    has_flags = 1;
                    // Ensure the flag contains only valid characters (d, f, e)
                    for (int j = 1; arg[j] != '\0'; j++)
                    {
                        if (arg[j] != 'd' && arg[j] != 'f' && arg[j] != 'e')
                        {
                            valid_flags = 0;
                            break;
                        }
                    }

                    if (!valid_flags)
                        break;

                    args[arg_count++] = arg; // Add flag to args
                }
                else if (!has_search_item) // The first non-flag argument is the search item
                {
                    has_search_item = 1;
                    args[arg_count++] = arg; // Add search item to args
                }
                else
                {
                    // If there are more arguments after the search item, it's an invalid command
                    printf("Invalid command format!\n");
                    return;
                }

                arg = strtok(NULL, " ");
            }

            // Check for valid format: either <flags> <search item> or <search item>
            if (!valid_flags)
            {
                printf("Invalid flags!\n");
            }
            else if (!has_search_item)
            {
                printf("Invalid command: missing search item!\n");
            }
            else
            {
                // Call the seek function with the parsed arguments
                seek(arg_count, (char **)args, home, currentdir);
            }
        }
        else if (strncmp(os, "proclore", 8) == 0)
        {
            char *pid_str = os + 8;
            while (*pid_str == ' ')
                pid_str++;
            int pid = (*pid_str == '\0') ? getpid() : atoi(pid_str);
            proclore(pid);
        }
        else if (strncmp(os, "iman", 4) == 0)
        {
            char *x = os + 4;
            while (*x == ' ')
                x++;
            char *args[3];
            args[0] = "iman";
            args[1] = "man";
            args[2] = x;
            iman(args, 3);
        }
        else if (strncmp(os, "exit", 4) == 0)
        {
            exit(0);
        }
        else if (strncmp(os, "log purge", 9) == 0)
        {
            *flag = 0;
            purge_log(log, log_count, currentdir, home);
        }
        else if (strncmp(os, "log execute", 11) == 0)
        {
            *flag = 0;
            int index = atoi(os + 12);
            *last_cmd_time = logexec(log, *log_count, index, home, *prevdir, currentdir);
            
           
        }
        else if (strncmp(os, "log", 3) == 0)
        {
            *flag = 0;

            printentries(log, *log_count);
        }
        else if (strncmp(os, "activities", 10) == 0)
        {
            activities();
        }
        else if (strncmp(os, "ping", 4) == 0)
        {
            char *cmd = strtok(os, " ");
            char *pid_str = strtok(NULL, " ");
            char *signal_str = strtok(NULL, " ");

            if (pid_str && signal_str)
            {
                int pid = atoi(pid_str);
                int signal_number = atoi(signal_str);

                ping_process(pid, signal_number);
            }
            else
            {
                printf("Usage: ping <pid> <signal_number>\n");
            }
        }
        else if (strncmp(os, "fg", 2) == 0)
        {
            int pid = atoi(os + 3);
            bring_to_foreground(pid);
        }
        else if (strncmp(os, "bg", 2) == 0)
        {
            int pid = atoi(os + 3);
            resume_in_background(pid);
        }
        else if (strncmp(os, "neonate-n", 9) == 0)
        {
            char *neonate_arg = os + 9;
            while (*neonate_arg == ' ')
                neonate_arg++;
            int time_arg = atoi(neonate_arg);
            neonate_n_command(time_arg);
        }
        else
        {
            char *input_file = NULL;
            char *output_file = NULL;
            int append = 0;
            char *redir_in = strchr(os, '<');
            char *redir_i = strchr(os, '<');
            if (redir_in)
            {
                *redir_in = '\0';
                redir_in++;
                while (*redir_in && isspace(*redir_in))
                {
                    redir_in++;
                }
                while (*redir_in && !isspace(*redir_in))
                {
                    redir_in++;
                }
                while (*redir_in && isspace(*redir_in))
                {
                    redir_in++;
                }
                strcat(os, redir_in);
                char *check = strchr(os, '<');
                if (check)
                {
                    printf("Invalid command\n");
                    return;
                }
            }

            if (redir_in)
            {
                input_file = strtok(redir_i + 1, " ");
            }
            pid_t pid = fork();
            if (pid == 0)
            {
                foreground_pid = getpid();
                if (input_file)
                {
                    int fd_in = open(input_file, O_RDONLY);
                    if (fd_in < 0)
                    {
                        perror("Error opening input file");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }
                char *args[100];
                char *arg = strtok(os, " ");
                int i = 0;
                while (arg != NULL)
                {
                    args[i++] = arg;
                    arg = strtok(NULL, " ");
                }
                args[i] = NULL;
                setpgid(0, 0);
                if (execvp(args[0], args) < 0)
                {
                    printf("ERROR: Command '%s' failed to execute\n", fg_command);
                }

                exit(0);
            }
            else
            {
                foreground_pid = pid;
                add_process(fg_command, pid, "Running");
                int status;
                waitpid(pid, &status, WUNTRACED);

                if (WIFSTOPPED(status))
                {
                    printf("Process [%d] stopped\n", pid);
                    update_process_state(pid, "Stopped");
                }
                else
                {
                    remove_process(pid);

                    foreground_pid = -1;
                }
                time_t end_time = time(NULL);

                int time_taken = (int)(end_time - start_time);
                *last_cmd_time = time_taken;
                if (time_taken > 2)
                {
                    strncpy(last_cmd, fg_command, 6000 - 1);
                }
            }
        }
    }
}

void pipe_it(char *instr, char **instr_strings)
{
    if (instr[0] == '|' || instr[strlen(instr) - 1] == '|')
    {
        printf("INVALID FORMAT\n");
        return;
    }
    char *token = strtok(instr, "|");
    while (token != NULL)
    {
        /* code */
        if (token != NULL)
        {
            instr_strings[l] = (char *)malloc(sizeof(char) * strlen(token) + 1);
            strcpy(instr_strings[l], token);
            l++;
        }
        token = strtok(NULL, "|");
    }
}

void pipings(char *string, char *home, char *prevdir, char *currentdir, char log[LOG_SIZE][6000], int *log_count, int *last_cmd_time, char *last_cmd, int *flag, int *pf)
{
    l = 0;
    char *pipe_strings[4096];
    pipe_it(string, pipe_strings);
    int l1 = dup(STDIN_FILENO);
    int l2 = dup(STDOUT_FILENO);
    int pipe_fd[2];
    int prev_fd = -1;
    for (int i = 0; i < l; i++)
    {
        char tken[4096];
        if (pipe_strings[i] == NULL)
        {
            printf("INVALID FORMAT\n");
            return;
        }
        memset(tken, '\0', sizeof(tken));
        strcpy(tken, pipe_strings[i]);
        char *token;
        token = strtok(tken, " ");
        if (token == NULL)
        {
            printf("INVALID FORMAT\n");
            return;
        }
    }
    for (int i = 0; i < l; i++)
    {

        if (i < l - 1)
        {

            if (pipe(pipe_fd) == -1)
            {
                perror("pipe");
                return;
            }
        }

        int forki = fork();
        if (forki < 0)
        {
            perror("fork");
            return;
        }
        else if (forki == 0)
        {
            if (prev_fd != -1)
            {
                if (dup2(prev_fd, STDIN_FILENO) < 0)
                {
                    perror("dup2");
                    exit(1);
                }
                close(prev_fd);
            }

            if (i < l - 1)
            {
                if (dup2(pipe_fd[1], STDOUT_FILENO) < 0)
                {
                    perror("dup2");
                    exit(1);
                }
                close(pipe_fd[1]);
            }

            close(pipe_fd[0]);
            char *t2 = pipe_strings[i];
            while (*t2 == ' ')
            {
                t2++;
            }
            char *output_redir = strchr(t2, '>');
            int output_fd = -1;
            int append_mode;
            char *output_filename;
            if (output_redir)
            {
                append_mode = 0;
                if (*(output_redir + 1) == '>')
                {
                    append_mode = 1;
                    *output_redir = '\0';
                    output_redir++;
                }
                *output_redir = '\0';
                output_redir++;
                while (*output_redir == ' ')
                    output_redir++;

                output_filename = strtok(output_redir, " ");

                if (output_filename)
                {
                    if (append_mode)
                    {
                        output_fd = open(output_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    }
                    else
                    {
                        output_fd = open(output_filename, O_WRONLY | O_CREAT, 0644);
                    }
                    if (output_fd == -1)
                    {
                        perror("Failed to open output file");
                    }
                }

                char *remaining_command = strtok(NULL, "");

                if (remaining_command)
                {
                    strcat(t2, " ");
                    strcat(t2, remaining_command);
                }
                char *end = t2 + strlen(t2) - 1;
                while (end > t2 && isspace((unsigned char)*end))
                {
                    end--;
                }
                *(end + 1) = '\0';
            }
            char *bg_command = strchr(t2, '&');
            while (bg_command != NULL)
            {

                *bg_command = '\0';
                bg_command++;
                while (*bg_command == ' ')
                    bg_command++;
                char *ios = strchr(t2, '>');
                if (ios)
                {
                    printf("INVALID COMMAND\n");
                }
                else
                {
                    if (output_fd != -1)
                    {
                        int saved_stdout = dup(STDOUT_FILENO);
                        dup2(output_fd, STDOUT_FILENO);
                        close(output_fd);
                        execute_background1(t2);
                        dup2(saved_stdout, STDOUT_FILENO);
                        close(saved_stdout);
                    }
                    else
                    {
                        execute_background1(t2);
                    }
                }

                t2 = bg_command;
                while (*t2 == ' ')
                    t2++;
                char *bg_command = strchr(t2, '&');
            }
            char *ios = strchr(t2, '>');
            if (ios)
            {
                printf("INVALID COMMAND\n");
            }
            else
            {
                if (output_fd != -1)
                {

                    int saved_stdout = dup(STDOUT_FILENO);
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                    getcwd(currentdir, sizeof(currentdir) - 1);
                    execute_foreground1(t2, home, &prevdir, currentdir, log, log_count, last_cmd_time, last_cmd, flag, pf);
                    dup2(saved_stdout, STDOUT_FILENO);
                    close(saved_stdout);
                }
                else
                {
                    execute_foreground1(t2, home, &prevdir, currentdir, log, log_count, last_cmd_time, last_cmd, flag, pf);
                }
            }
            exit(0);
        }
        else
        {
            wait(NULL);
            close(pipe_fd[1]);
            if (prev_fd != -1)
            {
                close(prev_fd);
            }
            prev_fd = pipe_fd[0];
        }
    }
    dup2(l1, STDIN_FILENO);
    dup2(l2, STDOUT_FILENO);
    close(l1);
    close(l2);
}

int main(int argc, char const *argv[])
{
    char user[6000];
    char lapname[6000];
    char currentdir[6000];
    char *prevdir = (char *)malloc(sizeof(char) * 10000);
    char home[6000];
    char log[LOG_SIZE][6000];
    int log_count = 0;
    int last_cmd_time = 0;
    char last_cmd[6000];
    char myshrc_path[6000];
    load_aliases(myshrc_path);
    load(log, &log_count);
    strncpy(user, getenv("USER"), sizeof(user) - 1);
    gethostname(lapname, sizeof(lapname) - 1);
    getcwd(home, sizeof(home));
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    while (1)
    {
        getcwd(currentdir, sizeof(currentdir) - 1);

        if (strncmp(currentdir, home, strlen(home)) == 0)
        {
            if (last_cmd_time > 2)
            {
                printf("<%s@%s:~%s %s : %ds> ", user, lapname, currentdir + strlen(home), last_cmd, last_cmd_time);
            }
            else
            {
                printf("<%s@%s:~%s> ", user, lapname, currentdir + strlen(home));
            }
        }
        else
        {
            if (last_cmd_time > 2)
            {
                printf("<%s@%s:%s %s : %ds> ", user, lapname, currentdir, last_cmd, last_cmd_time);
            }
            else
            {
                printf("<%s@%s:%s> ", user, lapname, currentdir);
            }
        }

        last_cmd[0] = '\0';
        last_cmd_time = 0;

        char cmd[6000];
        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
        {
            printf("\nStopping all running processes...\n");
            stop_all_running_processes();
            printf("Exiting shell...\n");
            exit(0);
        }
        char *newline = strchr(cmd, '\n');
        if (newline)
        {
            *newline = '\0';
        }
        char cmod[6000];
        strcpy(cmod, cmd);
        char *save1;
        char *comma = substitute_alias(cmd);
        // printf("comma=%s\n", comma);
        char *t2 = strtok_r(comma, ";", &save1);
        char *t22;
        t22 = t2;
        int flag = 1;
        while (t2 != NULL)
        {
            int leng = strlen(t2);
            getcwd(currentdir, sizeof(currentdir) - 1);
            int pf = 0;
            while (*t2 == ' ')
                t2++;
            if (strlen(t2) == 0)
            {
                t2 = strtok_r(NULL, ";", &save1);
                continue;
            }
            if (strncmp(t2, "log", 3) == 0)
            {
                flag = 0;
            }
            char *newb = strdup(t2);
            char *output_redir = strchr(newb, '>');
            int output_fd = -1;
            int append_mode;
            char *output_filename;
            if (output_redir)
            {
                append_mode = 0;
                if (*(output_redir + 1) == '>')
                {
                    append_mode = 1;
                    *output_redir = '\0';
                    output_redir++;
                }
                *output_redir = '\0';
                output_redir++;
                while (*output_redir == ' ')
                    output_redir++;

                output_filename = strtok(output_redir, " ");

                if (output_filename)
                {
                    if (append_mode)
                    {
                        output_fd = open(output_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    }
                    else
                    {
                        output_fd = open(output_filename, O_WRONLY | O_CREAT, 0644);
                    }
                    if (output_fd == -1)
                    {
                        perror("Failed to open output file");
                    }
                }

                char *remaining_command = strtok(NULL, "");

                if (remaining_command)
                {
                    strcat(newb, " ");
                    strcat(newb, remaining_command);
                }
                char *end = newb + strlen(newb) - 1;
                while (end > newb && isspace((unsigned char)*end))
                {
                    end--;
                }
                *(end + 1) = '\0';
            }
            char *bg_command = strchr(t2, '&');
            if (bg_command != NULL)
            {

                *bg_command = '\0';
                bg_command++;
                while (*bg_command == ' ')
                    bg_command++;
                char *ifpipe = strchr(t2, '|');
                if (ifpipe != NULL)
                {

                    char string[6000];
                    strcpy(string, t2);
                    strcat(string, " &");
                    printf("%s\n", string);
                    char *s1 = strstr(string, "||");
                    if (s1)
                    {
                        *s1 = '\0';

                        pipings(string, home, prevdir, currentdir, log, &log_count, &last_cmd_time, last_cmd, &flag, &pf);
                    }
                    else
                    {
                        pipings(string, home, prevdir, currentdir, log, &log_count, &last_cmd_time, last_cmd, &flag, &pf);
                    }
                }
                else
                {

                    char *bg = strchr(newb, '&');
                    *bg = '\0';
                    char *ios = strchr(newb, '>');
                    if (ios)
                    {
                        printf("INVALID COMMAND\n");
                    }
                    else
                    {
                        if (output_fd != -1)
                        {
                            // printf("newb=%s\n", newb);

                            int saved_stdout = dup(STDOUT_FILENO);
                            dup2(output_fd, STDOUT_FILENO);
                            close(output_fd);
                            execute_background1(newb);
                            dup2(saved_stdout, STDOUT_FILENO);
                            close(saved_stdout);
                        }
                        else
                        {

                            execute_background1(newb);
                        }
                    }
                }

                t2 = bg_command;
                while (*t2 == ' ')
                    t2++;
                continue;
            }

            char *ifpip = strchr(t2, '|');
            if (ifpip != NULL)
            {
                char *s1 = strstr(t2, "||");
                if (s1)
                {
                    *s1 = '\0';

                    pipings(t2, home, prevdir, currentdir, log, &log_count, &last_cmd_time, last_cmd, &flag, &pf);
                }
                else
                {
                    pipings(t2, home, prevdir, currentdir, log, &log_count, &last_cmd_time, last_cmd, &flag, &pf);
                }
            }
            else
            {

                char *ios = strchr(newb, '>');
                if (ios)
                {
                    printf("INVALID COMMAND\n");
                }
                else
                {
                    if (output_fd != -1)
                    {
                        char bose[10000];
                        strcpy(bose, newb);
                        int saved_stdout = dup(STDOUT_FILENO);
                        dup2(output_fd, STDOUT_FILENO);
                        close(output_fd);
                        getcwd(currentdir, sizeof(currentdir) - 1);
                        execute_foreground1(newb, home, &prevdir, currentdir, log, &log_count, &last_cmd_time, last_cmd, &flag, &pf);
                        if (strncmp(bose, "log execute", 11) == 0)
                        {

                            strcpy(t2, bose);
                        }
                        dup2(saved_stdout, STDOUT_FILENO);
                        close(saved_stdout);
                    }
                    else
                    {
                        char bose[10000];
                        strcpy(bose, newb);

                        execute_foreground1(newb, home, &prevdir, currentdir, log, &log_count, &last_cmd_time, last_cmd, &flag, &pf);
                        if (strncmp(bose, "log execute", 11) == 0)
                        {

                            strcpy(t2, bose);
                        }
                    }
                }
            }

            t2 = strtok_r(NULL, ";", &save1);

            while (t2 && *t2 == ' ')
                t2++;
        }
        if (flag == 1)
        {
            addentries(log, &log_count, cmod, currentdir, home);
        }
    }
    return 0;
}
