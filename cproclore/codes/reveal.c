#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "reveal.h"

void print_permissions(int mode)
{
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void print_file_info(const char *path, const char *filename, int show_hidden, int detailed)
{
    struct stat file_stat;
    char fullpath[6000];
    strcpy(fullpath, path);
    strcat(fullpath, "/");
    strcat(fullpath, filename);

    if (stat(fullpath, &file_stat) == -1)
    {
        printf(" error in copying the details to struct stat having pointer &file_stat\n");
        return;
    }

    if (!show_hidden && filename[0] == '.')
    {
        return;
    }

    if (detailed)
    {
        print_permissions(file_stat.st_mode);
        printf(" %ld", file_stat.st_nlink);

        struct passwd *pw = getpwuid(file_stat.st_uid);
        struct group *gr = getgrgid(file_stat.st_gid);

        printf(" %s %s", pw->pw_name, gr->gr_name);
        printf(" %5ld", file_stat.st_size);

        char timebuf[80];
        struct tm *timeinfo;
        timeinfo = localtime(&file_stat.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", timeinfo);
        printf(" %s", timebuf);
    }

    int use_colors = isatty(fileno(stdout)); 

    if (S_ISDIR(file_stat.st_mode))
    {
        if (use_colors)
        {
            printf("\033[1;34m %s\033[0m\n", filename); 
        }
        else
        {
            printf("%s\n", filename);
        }
    }
    else if (file_stat.st_mode & S_IXUSR)
    {
        if (use_colors)
        {
            printf("\033[1;32m %s\033[0m\n", filename); 
        }
        else
        {
            printf("%s\n", filename);
        }
    }
    else
    {
        if (use_colors)
        {
            printf("\033[1;37m %s\033[0m\n", filename); 
        }
        else
        {
            printf("%s\n", filename);
        }
    }
}

void reveal(const char *flags, const char *p, char *home, char *prevdir)
{
    DIR *d;
    struct dirent *dir;
    int hid = 0;
    int det = 0;
    char act[6000];

    while (*flags != '\0')
    {
        if (*flags == 'a')
            hid = 1;
        else if (*flags == 'l')
            det = 1;
        flags++;
    }

    if (strcmp(p, "~") == 0)
    {
        strncpy(act, home, sizeof(act));
    }
    else if (strcmp(p, "-") == 0)
    {
        if (strlen(prevdir) > 0)
        {
            strncpy(act, prevdir, sizeof(act));
        }
        else
        {
            printf("ERROR: No previous directory available\n");
            return;
        }
    }
    else if (strcmp(p, ".") == 0)
    {
        getcwd(act, sizeof(act));
    }
    else if (strcmp(p, "..") == 0)
    {
        char parent_path[6000];
        char original[6000];
        getcwd(original, sizeof(original));
        if (access("..", X_OK) == 0)
        {
            if (chdir("..") == 0)
            {
                getcwd(act, sizeof(act));
                chdir(original);
            }
            else
            {
                printf("reveal failed\n");
                return;
            }
        }
        else
        {
            printf("chdir permission denied\n");
        }
    }
    else
    {
        strncpy(act, p, sizeof(act));
    }

    struct stat file_stat;
    if (stat(act, &file_stat) == 0)
    {
        if (S_ISDIR(file_stat.st_mode))
        {

            d = opendir(act);
            if (!d)
            {
                printf("reveal failed");
                return;
            }

            struct dirent *entries[10000];
            int num_entries = 0;
            blkcnt_t total_blocks = 0;

            while ((dir = readdir(d)) != NULL)
            {
                if (hid || dir->d_name[0] != '.')
                {
                    entries[num_entries] = malloc(sizeof(struct dirent));
                    if (entries[num_entries] == NULL)
                    {
                        printf("malloc failed");
                        closedir(d);
                        return;
                    }
                    memcpy(entries[num_entries], dir, sizeof(struct dirent));
                    char fullpath[6000];
                    strcpy(fullpath, act);
                    strcat(fullpath, "/");
                    strcat(fullpath, dir->d_name);
                    if (stat(fullpath, &file_stat) == 0)
                    {
                        total_blocks += file_stat.st_blocks;
                    }
                    num_entries++;
                }
            }
            closedir(d);

            for (int i = 0; i < num_entries - 1; i++)
            {
                for (int j = i + 1; j < num_entries; j++)
                {
                    if (strcmp(entries[i]->d_name, entries[j]->d_name) > 0)
                    {
                        struct dirent *temp = entries[i];
                        entries[i] = entries[j];
                        entries[j] = temp;
                    }
                }
            }

            if (det)
            {
                printf("total %ld\n", total_blocks / 2);
            }
            for (int i = 0; i < num_entries; i++)
            {
                print_file_info(act, entries[i]->d_name, hid, det);
                free(entries[i]);
            }
        }
        else if (S_ISREG(file_stat.st_mode))
        {

            print_file_info(".", act, hid, det);
        }
        else
        {
            printf("ERROR: %s is not a regular file or directory\n", act);
        }
    }
    else
    {
        printf("reveal failed");
    }
}