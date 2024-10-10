#ifndef PATH_H
#define PATH_H

int is_dir(const char *dirName);
int create_dir(const char *dirName);
int get_bin_path(char *binPath, int size);
int traverse_up_path(char *buffer, int size, char *path,  int level);
int create_path(char *buffer, int size, const char *path);

#endif