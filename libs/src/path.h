#ifndef PATH_H
#define PATH_H

int is_dir(const char *dirName);
int create_dir(const char *dirName);

/* get path of the current executable */
int get_bin_path(char *binPath, int size);

/* modifies the path to move up a specified number of
    levels in the directory tree. Each slash in the
    path represents one level. 

    -example-
    path = "/usr/bin/bash"
    level: 1 
    result = "/usr/bin" */
int traverse_up_path(char *buffer, int size, char *path, int level);

/* create a new path by appending relative path (dir
    or file) to the current project directory. 

    -example-
    project dir = "/home/user/client"
    path = "/data" 
    result = "/home/user/client/data" */
int create_path(char *buffer, int size, const char *path);

#endif