#ifndef REVEAL_H
#define REVEAL_H

void print_permissions(int mode);
void print_file_info(const char *path, const char *filename, int show_hidden, int detailed);
void reveal(const char *flags, const char *p, char *home, char *prevdir);

#endif 
