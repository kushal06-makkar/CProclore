#ifndef IMAN_H
#define IMAN_H

// Define constants
#define BUFFER_SIZE 8192
#define PORT 80
#define MAX_COMMAND_LENGTH 256

// Function declarations
void fetch_man_page(const char *command);
void iman(char **instr, int k);

#endif
