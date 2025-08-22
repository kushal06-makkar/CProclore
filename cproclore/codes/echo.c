#include <stdio.h>
#include <string.h>
#include "echo.h"

int count_quotes(const char *str, char quote_char)
{
    int count = 0;
    while (*str)
    {
        if (*str == quote_char)
        {
            count++;
        }
        str++;
    }
    return count;
}

void ecko(char *command, int*pf)
{

   
    char *comment = strchr(command, '#');
    if (comment)
    {
        *comment = '\0';
    }

    char *start = command;
    while (*start == ' ')
    {
        start++;
    }
    char *end = start + strlen(start) - 1;
    while (end > start && *end == ' ')
    {
        end--;
    }
    *(end + 1) = '\0';

    int single_quote_count = count_quotes(start, '\'');
    int double_quote_count = count_quotes(start, '"');
    

    if (single_quote_count % 2 != 0 || double_quote_count % 2 != 0)
    {
        printf("ERROR: Unmatched quotes detected. Input ignored.\n");
        return;
    }

    int inside_single_quotes = 0;
    int inside_double_quotes = 0;
    for (char *c = start; *c != '\0'; c++)
    {
        if (*c == '\'')
        {
            inside_single_quotes = !inside_single_quotes;
        }
        else if (*c == '"')
        {
            inside_double_quotes = !inside_double_quotes;
        }
        else if (*c == '\\')
        {

            if (*(c + 1) == '\'' || *(c + 1) == '"')
            {
                c++;
                printf("%c", *c);
               
            }
            else
            {
                printf("%c", *c);
                
            }
        }
        else if (inside_single_quotes || inside_double_quotes || *c != ' ')
        {
            printf("%c", *c);
            
        }
       
        
    }
    putchar('\n');
}