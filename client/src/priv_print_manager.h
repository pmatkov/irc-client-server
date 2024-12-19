/* --INTERNAL HEADER--
    used for testing */
#ifndef PRINT_MANAGER_H
#define PRINT_MANAGER_H

#define NCURSES_WIDECHAR 1

#include "priv_base_window.h"

#include <ncursesw/curses.h>

typedef struct {
    int useTimestamp;
    const char *separator;
    const char *origin;
    const char *content;
    uint32_t format;
} MessageTokens;

MessageTokens * create_message_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format);
void delete_message_tokens(MessageTokens *messageTokens);

void print_tokens(BaseWindow *baseWindow, MessageTokens *messageTokens);
void print_tokens_xy(WINDOW *window, MessageTokens *messageTokens, int y, int x);
void print_cx_string(WINDOW *window, cchar_t *string, int y, int x);
void print_string(WINDOW *window, const char *string, int y, int x);

#ifdef TEST

/**
 * @brief Moves the cursor to the specified position in the given window.
 * 
 * @param window The window in which to move the cursor.
 * @param y The y-coordinate to move the cursor to.
 * @param x The x-coordinate to move the cursor to.
 */
void move_cursor(WINDOW *window, int y, int x);

/**
 * @brief Concatenates message tokens into a buffer.
 * 
 * @param buffer The buffer to store the concatenated message.
 * @param size The size of the buffer.
 * @param messageTokens The message tokens to concatenate.
 * @return int Returns 0 on success, or a non-zero error code on failure.
 */
int concat_msg_tokens(cchar_t *buffer, int size, MessageTokens *messageTokens);


#endif

#endif