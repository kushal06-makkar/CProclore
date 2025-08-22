#ifndef ACTIVITIES_H
#define ACTIVITIES_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define MAX_PROCESSES 100

typedef struct
{
    char command_name[256];
    pid_t pid;
    char state[10]; 
} ProcessInfo;

ProcessInfo *find_process_by_pid(pid_t pid);
void add_process(const char *command_name, pid_t pid, const char *state);
void update_process_state(pid_t pid, const char *state);
void stop_all_running_processes();
void remove_process(pid_t pid);
void activities(void);
void sigchld_handler(int signum);
void bring_to_foreground(pid_t pid);
void resume_in_background(pid_t pid);
void ping_process(int pid, int signal_number);

#endif // ACTIVITIES_H
