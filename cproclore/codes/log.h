#ifndef LOG_H
#define LOG_H

#define LOG_SIZE 15
#define LOG_FILE "/home/chaitu/Desktop/OSN/command_log.txt"

extern char last_cmd[6000];
extern int lxx;
void load(char log[LOG_SIZE][6000], int *log_count);
void saveentry(char log[LOG_SIZE][6000], int log_count, char *currentdir, char *home);
void printentries(char log[LOG_SIZE][6000], int log_count);
void addentries(char log[LOG_SIZE][6000], int *log_count, const char *command, char *currentdir, char *home);
void purge_log(char log[LOG_SIZE][6000], int *log_count, char *currentdir, char *home);
int logexec(char log[LOG_SIZE][6000], int log_count, int index, char *home, char *prevdir, char *currentdir);
void execute_background(char *bg_command);
void execute_foreground(char *fg_command, char *home, char **prevdir, char *currentdir, char log[LOG_SIZE][6000], int *log_count, int *last_cmd_time, char *last_cmd, int *flag, int *pf);
void pipe_it1(char *instr, char **instr_strings);
void pipings1(char *string, char *home, char *prevdir, char *currentdir, char log[LOG_SIZE][6000], int *log_count, int *last_cmd_time, char *last_cmd, int *flag, int *pf);


#endif
