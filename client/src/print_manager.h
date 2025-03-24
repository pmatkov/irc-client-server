#ifndef PRINT_MANAGER_H
#define PRINT_MANAGER_H

#define NCURSES_WIDECHAR 1

#include "base_window.h"

#include <ncursesw/curses.h>

/* a collection of tokens used for printing terminal messages */
typedef struct {
    int useTimestamp;
    const char *separator;
    const char *origin;
    const char *content;
    uint32_t format;
} MessageTokens;

MessageTokens * create_message_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format);
void delete_message_tokens(MessageTokens *messageTokens);
 
/* print tokens at the current cursor position, advance cursor to the 
    next line and save to scrollback */
void print_tokens(BaseWindow *baseWindow, MessageTokens *messageTokens);

/* print tokens at the specified cursor position and return to the 
    initial position */

void print_tokens_xy(WINDOW *window, MessageTokens *messageTokens, int y, int x);
 
/* print complex string at the specified position */
 void print_cx_string(WINDOW *window, cchar_t *string, int y, int x);

/* print regular string at the specified position */
void print_string(WINDOW *window, const char *string, int y, int x);

#endif