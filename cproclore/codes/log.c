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
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "hop.h"
#include "reveal.h"
#include "proclore.h"
#include "seek.h"
#include "echo.h"
#include "activities.h"
#include "alias.h"
#include "neonate.h"
#include "log.h"
#include "iman.h"

char last_cmd[6000];
int lxx = 0;
pid_t foreground_pid1 = -1;
void sigint_handler1(int signum)
{
    if (foreground_pid1 != -1)
    {
        kill(foreground_pid1, SIGINT);
    }
}
void sigtstp_handler1(int signum)
{
    if (foreground_pid1 != -1)
    {
        printf("\nStopping process %d\n", foreground_pid1);
        kill(foreground_pid1, SIGTSTP);
    }
}

void load(char log[LOG_SIZE][6000], int *log_count)
{
    FILE *fp = fopen(LOG_FILE, "r");
    if (fp)
    {
        while (fgets(log[*log_count], 6000, fp) != NULL && *log_count < LOG_SIZE)
        {
            log[*log_count][strcspn(log[*log_count], "\n")] = '\0';
            (*log_count)++;
        }
        fclose(fp);
    }
    else
    {
        printf("Error in reading the file\n");
    }
}

void saveentry(char log[LOG_SIZE][6000], int log_count, char *currentdir, char *home)
{
    FILE *fp = fopen(LOG_FILE, "w");
    if (fp)
    {
        for (int i = 0; i < log_count; i++)
        {
            fprintf(fp, "%s\n", log[i]);
        }
        fclose(fp);
    }
}

void printentries(char log[LOG_SIZE][6000], int log_count)
{
    for (int i = 0; i < log_count; i++)
    {
        printf("%s\n", log[i]);
    }
}

void addentries(char log[LOG_SIZE][6000], int *log_count, const char *command, char *currentdir, char *home)
{

    if (*log_count > 0 && strcmp(log[*log_count - 1], command) == 0)
    {
        return;
    }

    if (*log_count == LOG_SIZE)
    {
        for (int i = 1; i < LOG_SIZE; i++)
        {
            strcpy(log[i - 1], log[i]);
        }
        strcpy(log[LOG_SIZE - 1], command);
    }
    else
    {
        strcpy(log[*log_count], command);
        (*log_count)++;
    }
    saveentry(log, *log_count, currentdir, home);
}

void purge_log(char log[LOG_SIZE][6000], int *log_count, char *currentdir, char *home)
{
    *log_count = 0;
    saveentry(log, *log_count, currentdir, home);
}

void execute_background(char *bg_command)
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
void execute_foreground(char *fg_command, char *home, char **prevdir, char *currentdir, char log[LOG_SIZE][6000], int *log_count, int *last_cmd_time, char *last_cmd, int *flag, int *pf)
{
    if (fg_command && strlen(fg_command) > 0)
    {
        time_t start_time = time(NULL);
        char *nago = (char *)malloc(sizeof(char) * 6000);
        strcpy(nago, fg_command);
        char *redir_in = strchr(nago, '<');
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
                    strcat(nago, redir_in);
                }
                printf("fgcom=%s\n", nago);
                char *check = strchr(nago, '<');
                if (check)
                {
                    printf("Invalid command\n");
                    return;
                }
            }
        }

        if (strncmp(nago, "hop", 3) == 0)
        {

            char *x = nago + 3;
            while (*x == ' ')
                x++;
            hopcmd(x, home, *prevdir, NULL, pf);
            strcpy(*prevdir, currentdir);
        }
        else if (strncmp(nago, "echo", 4) == 0)
        {
            char *echo_cmd = strstr(nago, "echo");
            if (echo_cmd)
            {
                ecko(echo_cmd + 4, pf);
            }
        }
        else if (strncmp(nago, "reveal", 6) == 0)
        {

            char *x1 = nago + 6;
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
        else if (strncmp(nago, "seek", 4) == 0)
        {
            char *args[100];
            int arg_count = 0;
            char *arg = strtok(nago, " ");
            while (arg != NULL)
            {
                args[arg_count++] = arg;
                arg = strtok(NULL, " ");
            }
            seek(arg_count, (char **)args, home, currentdir);
        }
        else if (strncmp(nago, "proclore", 8) == 0)
        {
            char *pid_str = nago + 8;
            while (*pid_str == ' ')
                pid_str++;
            int pid = (*pid_str == '\0') ? getpid() : atoi(pid_str);
            proclore(pid);
        }
        else if (strncmp(nago, "exit", 4) == 0)
        {
            exit(0);
        }
        else if (strncmp(nago, "activities", 10) == 0)
        {
            activities();
        }
        else if (strncmp(nago, "iman", 4) == 0)
        {
            char *x = nago + 4;
            while (*x == ' ')
                x++;
            char *args[3];
            args[0] = "iman";
            args[1] = "man";
            args[2] = x;
            iman(args, 3);
        }
        else if (strncmp(nago, "ping", 4) == 0)
        {
            char *cmd = strtok(nago, " ");
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
        else if (strncmp(nago, "fg", 2) == 0)
        {
            int pid = atoi(nago + 3);
            bring_to_foreground(pid);
        }
        else if (strncmp(nago, "bg", 2) == 0)
        {
            int pid = atoi(nago + 3);
            resume_in_background(pid);
        }
        else if (strncmp(nago, "neonate-n", 9) == 0)
        {
            char *neonate_arg = nago + 9;
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
            char *redir_in = strchr(nago, '<');
            char *redir_i = strchr(nago, '<');
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
                strcat(nago, redir_in);
                char *check = strchr(nago, '<');
                if (check)
                {
                    printf("Invalid command\n");
                    // fg_command = NULL;
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
                foreground_pid1 = getpid();
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
                char *arg = strtok(nago, " ");
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
                foreground_pid1 = pid;
                add_process(nago, pid, "Running");
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

                    foreground_pid1 = -1;
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
        free(nago);
    }
}

void pipe_it1(char *instr, char **instr_strings)
{
    if (instr[0] == '|' || instr[strlen(instr) - 1] == '|')
    {
        printf("bba");

        printf("INVALID FORMAT\n");
        return;
    }
    char *token = strtok(instr, "|");
    while (token != NULL)
    {

        if (token != NULL)
        {
            instr_strings[lxx] = (char *)malloc(sizeof(char) * strlen(token) + 1);
            strcpy(instr_strings[lxx], token);
            lxx++;
        }
        token = strtok(NULL, "|");
    }
}

void pipings1(char *string, char *home, char *prevdir, char *currentdir, char log[LOG_SIZE][6000], int *log_count, int *last_cmd_time, char *last_cmd, int *flag, int *pf)
{
    lxx = 0;
    char *pipe_strings[4096];

    pipe_it1(string, pipe_strings);
    int l1 = dup(STDIN_FILENO);
    int l2 = dup(STDOUT_FILENO);
    int pipe_fd[2];
    int prev_fd = -1;
    for (int i = 0; i < lxx; i++)
    {
        char tken[4096];
        if (pipe_strings[i] == NULL)
        {
            printf("aaa");
            printf("INVALID FORMAT\n");
            return;
        }
        memset(tken, '\0', sizeof(tken));
        strcpy(tken, pipe_strings[i]);
        char *token;
        token = strtok(tken, " ");
        if (token == NULL)
        {
            printf("baa");
            printf("INVALID FORMAT\n");
            return;
        }
    }
    for (int i = 0; i < lxx; i++)
    {
        if (i < lxx - 1)
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

            if (i < lxx - 1)
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
                        execute_background(t2);
                        dup2(saved_stdout, STDOUT_FILENO);
                        close(saved_stdout);
                    }
                    else
                    {
                        execute_background(t2);
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
                    execute_foreground(t2, home, &prevdir, currentdir, log, log_count, last_cmd_time, last_cmd, flag, pf);
                    dup2(saved_stdout, STDOUT_FILENO);
                    close(saved_stdout);
                }
                else
                {
                    execute_foreground(t2, home, &prevdir, currentdir, log, log_count, last_cmd_time, last_cmd, flag, pf);
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

int logexec(char cmd_log[LOG_SIZE][6000], int cmd_count, int log_idx, char *home_dir, char *prev_directory, char *cur_directory)
{
    if (log_idx < 1 || log_idx > cmd_count)
    {
        printf("ERROR: Invalid log index\n");
        return 0;
    }
    signal(SIGCHLD,sigchld_handler);
    signal(SIGINT, sigint_handler1);
    signal(SIGTSTP, sigtstp_handler1);
    char original_cmd[4096];
    strcpy(original_cmd, cmd_log[cmd_count - log_idx]);
    char modified_cmd[6000];
    char curr_dir[6000];
    strcpy(modified_cmd, original_cmd);
    int last_cmd_exec_time = 0;
    char *newline_ptr = strchr(original_cmd, '\n');
    if (newline_ptr)
    {
        *newline_ptr = '\0';
    }
    strcpy(modified_cmd, original_cmd);
    char *alias_save;
    char *alias_subst = substitute_alias(original_cmd);
    char *semicolon_save;
    char *sub_cmd = strtok_r(alias_subst, ";", &semicolon_save);
    int cmd_status = 1;
    strcpy(curr_dir, cur_directory);
    while (sub_cmd != NULL)
    {
        getcwd(curr_dir, sizeof(curr_dir) - 1);
        int piped_flag = 0;
        while (*sub_cmd == ' ')
            sub_cmd++;
        if (strlen(sub_cmd) == 0)
        {
            sub_cmd = strtok_r(NULL, ";", &semicolon_save);
            continue;
        }
        char *temp_cmd = (char *)malloc(sizeof(char) * 6000);
        strcpy(temp_cmd, sub_cmd);
        char *output_redir = strchr(temp_cmd, '>');
        int out_fd = -1;
        int append_output_mode;
        char *output_file;
        if (output_redir)
        {
            append_output_mode = 0;
            if (*(output_redir + 1) == '>')
            {
                append_output_mode = 1;
                *output_redir = '\0';
                output_redir++;
            }
            *output_redir = '\0';
            output_redir++;
            while (*output_redir == ' ')
                output_redir++;

            output_file = strtok(output_redir, " ");

            if (output_file)
            {
                if (append_output_mode)
                {
                    out_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                }
                else
                {
                    out_fd = open(output_file, O_WRONLY | O_CREAT, 0644);
                }
                if (out_fd == -1)
                {
                    perror("Failed to open output file");
                }
            }

            char *remaining_cmd = strtok(NULL, "");

            if (remaining_cmd)
            {
                strcat(temp_cmd, " ");
                strcat(temp_cmd, remaining_cmd);
            }
            char *cmd_end = temp_cmd + strlen(temp_cmd) - 1;
            while (cmd_end > temp_cmd && isspace((unsigned char)*cmd_end))
            {
                cmd_end--;
            }
            *(cmd_end + 1) = '\0';
        }
        char *bg_flag = strchr(sub_cmd, '&');
        if (bg_flag != NULL)
        {

            *bg_flag = '\0';
            bg_flag++;
            while (*bg_flag == ' ')
                bg_flag++;
            char *io_error = strchr(temp_cmd, '>');
            if (io_error)
            {
                printf("INVALID COMMAND\n");
            }
            else
            {
                if (out_fd != -1)
                {
                    int saved_stdout_fd = dup(STDOUT_FILENO);
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                    execute_background(sub_cmd);
                    dup2(saved_stdout_fd, STDOUT_FILENO);
                    close(saved_stdout_fd);
                }
                else
                {
                    execute_background(sub_cmd);
                }
            }

            sub_cmd = bg_flag;
            while (*sub_cmd == ' ')
                sub_cmd++;
            continue;
        }

        char *pipe_flag = strchr(sub_cmd, '|');
        if (pipe_flag != NULL)
        {
            printf("%s\n", sub_cmd);
            pipings1(sub_cmd, home_dir, prev_directory, curr_dir, cmd_log, &cmd_count, &last_cmd_exec_time, last_cmd, &cmd_status, &piped_flag);
        }
        else
        {
            char *io_error = strchr(temp_cmd, '>');
            if (io_error)
            {
                printf("INVALID COMMAND\n");
            }
            else
            {
                if (out_fd != -1)
                {
                    int saved_stdout_fd = dup(STDOUT_FILENO);
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                    getcwd(curr_dir, sizeof(curr_dir) - 1);

                    execute_foreground(temp_cmd, home_dir, &prev_directory, cur_directory, cmd_log, &cmd_count, &last_cmd_exec_time, last_cmd, &cmd_status, &piped_flag);

                    dup2(saved_stdout_fd, STDOUT_FILENO);
                    close(saved_stdout_fd);
                    fflush(stdout);
                }
                else
                {
                    execute_foreground(temp_cmd, home_dir, &prev_directory, curr_dir, cmd_log, &cmd_count, &last_cmd_exec_time, last_cmd, &cmd_status, &piped_flag);
                }
            }
        }
        free(temp_cmd);
        sub_cmd = strtok_r(NULL, ";", &semicolon_save);
        while (sub_cmd && *sub_cmd == ' ')
            sub_cmd++;
    }
    sub_cmd = NULL;
    addentries(cmd_log, &cmd_count, modified_cmd, curr_dir, home_dir);

    return last_cmd_exec_time;
}
