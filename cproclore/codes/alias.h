#ifndef ALIAS_H
#define ALIAS_H

#define MAX_ALIASES 100

typedef struct
{
    char alias[50];
    char command[500];
} Alias;

extern Alias aliases[MAX_ALIASES];
extern int alias_count;

void load_aliases(const char *myshrc_path);
char *substitute_alias(char *cmd);

#endif
