#include "activities.h"
#include <stdio.h> 
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <stdlib.h>   
#include <errno.h>
ProcessInfo processes[MAX_PROCESSES];
int process_count = 0;


ProcessInfo *find_process_by_pid(pid_t pid)
{
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid == pid)
        {
            return &processes[i];
        }
    }
    return NULL; 
}

void add_process(const char *command_name, pid_t pid, const char *state)
{
    if (process_count < MAX_PROCESSES)
    {
        strncpy(processes[process_count].command_name, command_name, sizeof(processes[process_count].command_name) - 1);
        processes[process_count].pid = pid;
        strncpy(processes[process_count].state, state, sizeof(processes[process_count].state) - 1);
        process_count++;
    }
    else
    {
        printf("Process list is full!\n");
    }
}

void update_process_state(pid_t pid, const char *state)
{
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid == pid)
        {
            strncpy(processes[i].state, state, sizeof(processes[i].state) - 1);
            break;
        }
    }
}

void stop_all_running_processes()
{
    for (int i = 0; i < process_count; i++)
    {

        kill(processes[i].pid, SIGKILL);
        remove_process(processes[i].pid);
    }
}

void remove_process(pid_t pid)
{
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid == pid)
        {
            
            for (int j = i; j < process_count - 1; j++)
            {
                processes[j] = processes[j + 1];
            }
            process_count--; 
            break;
        }
    }
}

int compare_processes(const void *a, const void *b)
{
    ProcessInfo *p1 = (ProcessInfo *)a;
    ProcessInfo *p2 = (ProcessInfo *)b;
    return strcmp(p1->command_name, p2->command_name);
}

void activities(void)
{
    qsort(processes, process_count, sizeof(ProcessInfo), compare_processes);

    for (int i = 0; i < process_count; i++)
    {
        printf("%d : %s - %s\n", processes[i].pid, processes[i].command_name, processes[i].state);
    }
}

void sigchld_handler(int signum)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        if (WIFSTOPPED(status))
        {
            

            update_process_state(pid, "Stopped");
        }
        else if (WIFEXITED(status))
        {
           
            int exit_status = WEXITSTATUS(status); 
            if (exit_status == EXIT_FAILURE)
            {
                printf("Process %d exited with failure (status: %d)\n", pid, exit_status);
            }
            else
            {
                printf("Process exited normally with status: %d\n", WEXITSTATUS(status));
            }
            remove_process(pid); 
        }
        else if (WIFSIGNALED(status))
        {
           
            int signal_number = WTERMSIG(status); 
            printf("Process %d was terminated by signal %d\n", pid, signal_number);
            remove_process(pid); 
        }
    }
}

void bring_to_foreground(pid_t pid)
{
  
    ProcessInfo *proc = find_process_by_pid(pid);

    if (proc == NULL)
    {
        printf("No such process found.\n");
        return;
    }
   
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    
    if (tcsetpgrp(STDIN_FILENO, pid) == -1)
    {
        perror("Failed to bring process to foreground");
        
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        return;
    }

   
    if (strcmp(proc->state, "Stopped") == 0)
    {
        if (kill(pid, SIGCONT) == -1)
        {
            perror("Failed to continue process");
            tcsetpgrp(STDIN_FILENO, getpid()); 
           
            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            return;
        }

        update_process_state(pid, "Running");
    }

    

    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1)
    {
        perror("Failed to wait for process");
        tcsetpgrp(STDIN_FILENO, getpid()); 
       
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        return;
    }

  
    if (tcsetpgrp(STDIN_FILENO, getpid()) == -1)
    {
        perror("Failed to restore control to shell");
    }

    
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    
    if (WIFSTOPPED(status))
    {
        update_process_state(pid, "Stopped");

        printf("Process [%d] was stopped by signal %d\n", pid, WSTOPSIG(status));
    }
    else if (WIFEXITED(status))
    {
        remove_process(pid);

        printf("Process [%d] exited with status %d\n", pid, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        update_process_state(pid, "Stopped");
        printf("Process [%d] was killed by signal %d\n", pid, WTERMSIG(status));
    }
    else if (WIFCONTINUED(status))
    {
        remove_process(pid);
        printf("Process [%d] continued\n", pid);
    }
}

void resume_in_background(pid_t pid)
{
    ProcessInfo *process = find_process_by_pid(pid);
    if (process != NULL)
    {
        
        kill(process->pid, SIGCONT); 
        
        update_process_state(process->pid, "Running");

        
        printf("Process %d (%s) resumed in background.\n", process->pid, process->command_name);
    }
    else
    {
        printf("Process with PID %d not found!\n", pid);
    }
}

void ping_process(int pid, int signal_number)
{
    signal_number = signal_number % 32;

    if (kill(pid, 0) == -1)
    {
        if (errno == ESRCH)
        {
            printf("No such process found\n");
            return;
        }
    }

   
    if (kill(pid, signal_number) == 0)
    {
        printf("Sent signal %d to process with pid %d\n", signal_number, pid);
    }
    else
    {
        perror("Failed to send signal");
    }
    char proc_file[256];
    char buffer[512];
    snprintf(proc_file, sizeof(proc_file), "/proc/%d/status", pid);
    FILE *f = fopen(proc_file, "r");
    if (f == NULL)
    {

        update_process_state(pid, "Stopped");
    }
    while (fgets(buffer, sizeof(buffer), f))
    {

        if (strncmp(buffer, "State:", 6) == 0)
        {
            char state = buffer[7];

            switch (state)
            {
            case 'R':
                update_process_state(pid, "Stopped");

                break;
            case 'S':
            case 'D':
                update_process_state(pid, "Rinning");
                break;
            case 'T':
                update_process_state(pid, "Stopped");

                break;
            case 'Z':
                update_process_state(pid, "Zombie");

                break;
            default:
                update_process_state(pid, "Unknown");

                break;
            }
            break;
        }
    }

    fclose(f);
}