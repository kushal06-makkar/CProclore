#ifndef SEEK_H
#define SEEK_H

void search_directory(const char *dir_path, const char *search, int flag_d, int flag_e, int flag_f, const char *start_path, int *matches, char *single_match_path, int *single_match_is_dir);

void seek(int argc, char *argv[], char *home, char *currentdir);

#endif
