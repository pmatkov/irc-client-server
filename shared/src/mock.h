#ifndef MOCKS_H
#define MOCKS_H

#include <stddef.h>
#include <sys/types.h>
#include <ncursesw/curses.h>

#define read mock_read
#define write mock_write
#define close mock_close

extern int mockFd;
extern void * mockBuffP;
extern size_t mockLen;

ssize_t mock_read(int fd, void *buffer, size_t len);
ssize_t mock_write(int fd, const void *buffer, size_t len);
int mock_close(int fd);

#endif