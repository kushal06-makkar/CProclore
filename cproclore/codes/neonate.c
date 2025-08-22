#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <ctype.h>
#include "neonate.h"


void set_non_blocking_mode()
{
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); 

    if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
    }
}


void restore_terminal_mode()
{
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    oldt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK) == -1)
    {
        perror("fcntl");
    }
}
int get_latest_pid()
{
    DIR *proc = opendir("/proc");
    if (!proc)
    {
        perror("Cannot open /proc directory");
        return -1;
    }

    struct dirent *entry;
    int latest_pid = -1;
    time_t latest_time = 0;

    while ((entry = readdir(proc)) != NULL)
    {
        
        if (atoi(entry->d_name) > 0)
        {
            char path[6000];
            snprintf(path, sizeof(path), "/proc/%s", entry->d_name);

            struct stat st;
            if (stat(path, &st) == 0)
            {
                
                if (st.st_ctime > latest_time)
                {
                    latest_time = st.st_ctime;
                    latest_pid = atoi(entry->d_name);
                }
            }
        }
    }

    closedir(proc);
    return latest_pid;
}


void neonate_n_command(int time_arg)
{
    set_non_blocking_mode();

    fd_set read_fds;
    struct timeval tv;
    int max_fd = STDIN_FILENO;

    while (1)
    {
        
        int latest_pid = get_latest_pid();
        if (latest_pid != -1)
        {
            printf("%d\n", latest_pid);
        }

       
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

       
        tv.tv_sec = time_arg;
        tv.tv_usec = 0;

       
        int retval = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

        if (retval == -1)
        {
            perror("select()");
            break;
        }
        else if (retval > 0 && FD_ISSET(STDIN_FILENO, &read_fds))
        {
            
            char ch;
            if (read(STDIN_FILENO, &ch, 1) > 0 && ch == 'x')
            {
                break;
            }
        }
    }

    restore_terminal_mode();
    printf("\nNeonate command stopped.\n");
}
