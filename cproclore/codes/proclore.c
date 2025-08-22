#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "proclore.h"


void proclore(int pid)
{
    char path[6000];
    char buffer[6000];
    int fd;

    char pid_str[100];
    sprintf(pid_str, "%d", pid);

    strcpy(path, "/proc/");
    strcat(path, pid_str);
    strcat(path, "/stat");

    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("Failed to open stat file\n");
        return;
    }

    int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0)
    {
        printf("Failed to read stat file\n");
        close(fd);
        return;
    }
    buffer[bytesRead] = '\0';
    close(fd);

    char state;
    int pgrp;
    sscanf(buffer, "%*d %*s %c %*d %d", &state, &pgrp);

   
    int shell_pgrp = tcgetpgrp(STDOUT_FILENO);

    char statestr[3];
    statestr[0] = state;
    statestr[1] = (pgrp == shell_pgrp) ? '+' : '\0';
    statestr[2] = '\0';
    



    strcpy(path, "/proc/");
    strcat(path, pid_str);
    strcat(path, "/status");
    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("Failed to open status file");
        return;
    }

    char *vm_size_str = NULL;
    int read_size;
    while ((read_size = read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[read_size] = '\0';
        if ((vm_size_str = strstr(buffer, "VmSize:")) != NULL)
        {
            break;
        }
    }
    close(fd);

    char vm_size[32];
    if (vm_size_str != NULL)
    {
        sscanf(vm_size_str, "VmSize: %s", vm_size);
    }
    else
    {
        strcpy(vm_size, "0");
    }

   
    strcpy(path, "/proc/");
    strcat(path, pid_str);
    strcat(path, "/exe");
    char exe_path[6000];
    int exe_path_len = readlink(path, exe_path, sizeof(exe_path) - 1);
    if (exe_path_len != -1)
    {
        exe_path[exe_path_len] = '\0';
    }
    else
    {
        strcpy(exe_path, " ");
    }

    printf("pid: %d\n", pid);
    printf("process status: %s\n", statestr);
    printf("Process Group: %d\n", pgrp);
    printf("Virtual memory: %s kB\n", vm_size);
    printf("Executable path: %s\n", exe_path);
    
}