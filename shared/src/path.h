#ifndef PATH_H
#define PATH_H

int is_dir(const char *dirName);
int create_dir(const char *dirName);
int get_bin_path(char *binPath, int size);
int traverse_up_path(char *output, int size, char *path,  int level);
int set_default_path(char *output, int size, const char *path);

#endif