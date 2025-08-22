#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "seek.h"

void search_directory(const char *dir_path, const char *search, int flag_d, int flag_e, int flag_f, const char *start_path, int *matches, char *single_match_path, int *single_match_is_dir)
{
    DIR *dir;
    struct dirent *entry;
    char path[6000];
    struct stat statbuf;

    if (!(dir = opendir(dir_path)))
    {
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        strcpy(path, dir_path);
        strcat(path, "/");
        strcat(path, entry->d_name);
        if (lstat(path, &statbuf) == -1)
        {
           
            continue;
        }

        int is_dir = S_ISDIR(statbuf.st_mode);

        if (strncmp(entry->d_name, search, strlen(search)) == 0)
        {
            if ((is_dir && !flag_f) || (!is_dir && !flag_d))
            {
                (*matches)++;

                if (*matches == 1)
                {
                    strcpy(single_match_path, path);
                    *single_match_is_dir = is_dir;
                }
                char relative_path[6000];
                strcpy(relative_path, ".");                           
                strcat(relative_path, dir_path + strlen(start_path));
                strcat(relative_path, "/");                           
                strcat(relative_path, entry->d_name);

                if (is_dir)
                {
                    printf("\033[1;34m%s\033[0m\n", relative_path);
                }
                else
                {
                    printf("\033[1;32m%s\033[0m\n", relative_path);
                }
            }
        }

        if (is_dir && !S_ISLNK(statbuf.st_mode))
        {
            search_directory(path, search, flag_d, flag_e, flag_f, start_path, matches, single_match_path, single_match_is_dir);
        }
    }
    closedir(dir);
}

void seek(int argc, char *argv[], char *home, char *currentdir)
{
    int flag_d = 0, flag_f = 0, flag_e = 0;
    char *search = NULL, *target_dir = NULL;
    char full_path[6000];
    int i;
    
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (strchr(argv[i], 'd'))
                flag_d = 1;
            if (strchr(argv[i], 'f'))
                flag_f = 1;
            if (strchr(argv[i], 'e'))
                flag_e = 1;
        }
        else if (!search&&argv[i]!=NULL)
        {
            search = argv[i];
        }
        else if (!target_dir&&argv[i]!=NULL)
        {
            target_dir = argv[i];
        }
    }
    
   
    if (flag_d && flag_f)
    {
        printf("Invalid flags!\n");
        return;
    }

    if (!target_dir)
    {
        target_dir = currentdir;
    }

    if (target_dir[0] == '~')
    {

        strcpy(full_path, home);
        strcat(full_path, target_dir + 1);
    }
    else if (target_dir[0] != '/')
    {
        strcpy(full_path, currentdir);
        strcat(full_path, "/");
        strcat(full_path, target_dir);
       
    }
    else
    {
        
        strcpy(full_path, target_dir);
    }

    int matches = 0;
    char single_match_path[6000] = {0};
    int single_match_is_dir = 0;

    search_directory(full_path, search, flag_d, flag_e, flag_f, full_path, &matches, single_match_path, &single_match_is_dir);

    if (matches == 0)
    {
        printf("No match found!\n");
    }
    else if (matches == 1 && flag_e)
    {

        if (single_match_is_dir)
        {
            if (access(single_match_path, X_OK) == 0)
            {
                int dd2 = chdir(single_match_path);
                if (dd2 != 0)
                {
                    printf("Hop command failed\n");

                    return;
                }
            }
            else
            {
                printf("Missing permissions for chdir!\n");
            }
        }
        else
        {
            if (access(single_match_path, R_OK) == 0)
            {
                FILE *file = fopen(single_match_path, "r");
                if (file)
                {
                    char line[256];
                    while (fgets(line, sizeof(line), file))
                    {
                        printf("%s", line);
                    }
                    fclose(file);
                }
            }
            else
            {
                printf("Missing permissions for reading!\n");
            }
        }
    }
}
