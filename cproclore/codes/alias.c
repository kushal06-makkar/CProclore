#include "alias.h"
#include <stdio.h>
#include <string.h>

Alias aliases[MAX_ALIASES];
int alias_count = 0;

void load_aliases(const char *myshrc_path)
{
    FILE *file = fopen("/home/chaitu/Desktop/OSN/MiniProject-1/codes/.myshrc", "r");
    if (!file)
    {
        perror("Failed to open .myshrc");
        return;
    }

    char line[550];
    while (fgets(line, sizeof(line), file))
    {
        
        char *trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t')
        {
            trimmed_line++;
        }

        
        if (trimmed_line[0] == '#' || trimmed_line[0] == '\n')
        {
            continue;
        }

        
        if (strstr(trimmed_line, "mk_hop") != NULL)
        {
            strcpy(aliases[alias_count].alias, "mk_hop");
            strcpy(aliases[alias_count].command, "mkdir ; hop");
            alias_count++;
        }
        else if (strstr(trimmed_line, "hop_seek") != NULL)
        {
            strcpy(aliases[alias_count].alias, "hop_seek");
            strcpy(aliases[alias_count].command, "hop ; seek");
            alias_count++;
        }
        else
        {
           
            char *alias = strtok(trimmed_line, "=");
            char *command = strtok(NULL, "\n");

            if (alias && command)
            {
               
                char *comment_pos = strstr(command, "#");
                if (comment_pos != NULL)
                {
                    *comment_pos = '\0'; 
                }

                strcpy(aliases[alias_count].alias, alias);
                strcpy(aliases[alias_count].command, command);
                alias_count++;
            }
        }
    }
    fclose(file);
}

char *substitute_alias(char *cmd)
{
    static char substituted_cmd[6000];
    substituted_cmd[0] = '\0';

    
    char *comment_pos = strstr(cmd, "#");
    if (comment_pos != NULL)
    {
        *comment_pos = '\0'; 
    }

    char *token;
    char *saveptr;

    token = strtok_r(cmd, " ", &saveptr);

    while (token != NULL)
    {
        int alias_found = 0;
        for (int i = 0; i < alias_count; i++)
        {
            if (strcmp(token, aliases[i].alias) == 0)
            {
                if (strcmp(aliases[i].alias, "mk_hop") == 0)
                {
                    char *arg = strtok_r(NULL, " ", &saveptr);
                    if (arg != NULL)
                    {
                        snprintf(substituted_cmd, sizeof(substituted_cmd), "mkdir %s ; hop %s", arg, arg);
                    }
                    alias_found = 1;
                    break;
                }
                else if (strcmp(aliases[i].alias, "hop_seek") == 0)
                {
                    char *arg = strtok_r(NULL, " ", &saveptr);
                    if (arg != NULL)
                    {
                        snprintf(substituted_cmd, sizeof(substituted_cmd), "hop %s ; seek %s", arg, arg);
                    }
                    alias_found = 1;
                    break;
                }
                else
                {
                    strcat(substituted_cmd, aliases[i].command);
                    alias_found = 1;
                    break;
                }
            }
        }

        if (!alias_found)
        {
            strcat(substituted_cmd, token);
        }

        token = strtok_r(NULL, " ", &saveptr);
        if (token != NULL)
        {
            strcat(substituted_cmd, " ");
        }
    }
    printf("%s\n", substituted_cmd);
    return substituted_cmd;
}
