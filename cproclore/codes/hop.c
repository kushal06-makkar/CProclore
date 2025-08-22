#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hop.h"

void hopcmd(char *x, char *home, char *prevdir,char**pipe,int*pf)
{
    char actualdir[6000];
    getcwd(actualdir, sizeof(actualdir));

    char *t = strtok(x, " ");
    if (t == NULL)
    {
        chdir(home);
    }
    while (t != NULL)
    {
        if (strncmp(t, "~/", 2) == 0)
        {

            strcpy(actualdir, home);
            strcat(actualdir, t + 1);
            if (access(actualdir, X_OK) == 0)
            {
                if (chdir(actualdir) != 0)
                {
                    printf("Hop command failed\n");
                    return;
                }
            }
            else
            {
                printf("Permission Denied\n");
            }
        }
        else if (strcmp(t, "~") == 0)
        {
            if (access(home, X_OK) == 0)
            {
                if (chdir(home) != 0)
                {
                    printf("Hop command failed\n");
                    return;
                }
            }
            else
            {
                printf("Permission denied\n");
            }
        }
        else if (strcmp(t, "-") == 0)
        {
            if (strlen(prevdir) > 0)
            {
                // printf("%s\n",prevdir);
                if (access(prevdir, X_OK) == 0)
                {
                    int dd = chdir(prevdir);
                    if (dd != 0)
                    {
                        printf("Hop command failed\n");

                        return;
                    }
                }
                else
                {
                    printf("hop permission denied\n");
                }
            }
            else
            {
                printf("ERROR: No previous directory\n");
            }
        }
        else if (strcmp(t, ".") == 0)
        {
            // printf("%s\n", actualdir);
        }

        else if (strcmp(t, "..") == 0)
        {
            if (access("..", X_OK) == 0)
            {
                int dd2 = chdir("..");
                if (dd2 != 0)
                {
                    printf("Hop command failed\n");

                    return;
                }
            }
            else
            {
                printf("hop permision denied\n");
            }
        }
        else
        {
            if (access(t, X_OK) == 0)
            {
                int dd1 = chdir(t);
                if (dd1 != 0)
                {
                    printf("Hop command failed\n");
                    return;
                }
            }
        }

        getcwd(actualdir, sizeof(actualdir));
        printf("%s\n", actualdir);
        if(*pf==1)
        {
        strcpy(*pipe,actualdir);
        }
        t = strtok(NULL, " ");
    }
}