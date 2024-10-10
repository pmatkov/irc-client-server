#include "path.h"
#include "error_control.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH 64

int is_dir(const char *dirName) {

    int isdir = 0;

    DIR* dir = opendir(dirName);

    if (dir != NULL) {
        closedir(dir);
        isdir = 1;
    }
    return isdir;
}

int create_dir(const char *dirName) {

    return mkdir(dirName, 0700);   
}

int get_bin_path(char *binPath, int size) {

    ssize_t len;
    int status = 0;

    len = readlink("/proc/self/exe", binPath, size - 1);

    if (len != -1) {
        binPath[len] = '\0';
        status = 1;
    }

    return status;
}

// traverse up dir path
int traverse_up_path(char *buffer, int size, char *path,  int level) {

    if (path == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char *pathCopy = strdup(path);
    char *pathPtr = NULL;
    int status = 0;

    while (level-- && ((pathPtr = strrchr(pathCopy, '/')) != NULL && pathPtr != pathCopy)) {
        *pathPtr = '\0';
    }

    if (pathPtr != NULL && pathPtr != pathCopy) {

        int pathLen = pathPtr - pathCopy;
        if (pathLen > size - 1) {
            pathLen = size - 1;
        }
        strncpy(buffer, pathCopy, pathLen);
        buffer[pathLen] = '\0';
        status = 1;
    }

    free(pathCopy);

    return status;
}

int create_path(char *buffer, int size, const char *path) {

    if (buffer == NULL || path == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char binPath[MAX_PATH + 1] = {'\0'};
    int level = 2;
    int status = 0;

    #ifdef TEST
    level = 3;
    #endif

    if (get_bin_path(binPath, MAX_PATH) && traverse_up_path(buffer, MAX_PATH, binPath, level)) {

        if (strlen(path) + strlen(buffer) < size) {

            strcat(buffer, path);
            status = 1;
        }
    }

    return status;
}