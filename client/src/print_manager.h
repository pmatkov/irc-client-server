#ifndef PRINT_MANAGER_H
#define PRINT_MANAGER_H

#define NCURSES_WIDECHAR 1

#include "base_window.h"

#include <ncursesw/curses.h>

/**
 * @brief A collection of tokens used for printing terminal messages.
 *
 * The MessageTokens struct encapsulates various elements that can be used
 * to format and display messages in a terminal, including options for
 * timestamps, separators, origins, content, and formatting styles.
 */
typedef struct {
    int useTimestamp;
    const char *separator;
    const char *origin;
    const char *content;
    uint32_t format;
} MessageTokens;

/**
 * @brief Create message tokens.
 * 
 * @param useTimestamp Flag to include timestamp.
 * @param separator Separator string.
 * @param origin Origin of the message.
 * @param content Message content.
 * @param format Message format.
 * @return Pointer to created MessageTokens.
 */
MessageTokens * create_message_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format);
 
/**
 * @brief Delete message tokens.
 * 
 * @param messageTokens Pointer to MessageTokens to delete.
 */
void delete_message_tokens(MessageTokens *messageTokens);
 
/**
 * @brief Print tokens at the current cursor position, advance cursor to the next line and save to scrollback.
 * 
 * @param baseWindow Pointer to BaseWindow.
 * @param messageTokens Pointer to MessageTokens to print.
 */
void print_tokens(BaseWindow *baseWindow, MessageTokens *messageTokens);

/**
 * @brief Print tokens at the specified cursor position and return to the initial position.
 * 
 * @param window Pointer to WINDOW.
 * @param messageTokens Pointer to MessageTokens to print.
 * @param y Y-coordinate.
 * @param x X-coordinate.
 */

void print_tokens_xy(WINDOW *window, MessageTokens *messageTokens, int y, int x);
 
/**
 * @brief Print complex string at the specified position.
 * 
 * @param window Pointer to WINDOW.
 * @param string Pointer to cchar_t string to print.
 * @param y Y-coordinate.
 * @param x X-coordinate.
 */
 void print_cx_string(WINDOW *window, cchar_t *string, int y, int x);

/**
 * @brief Print regular string at the specified position.
 * 
 * @param window Pointer to WINDOW.
 * @param string Pointer to string to print.
 * @param y Y-coordinate.
 * @param x X-coordinate.
 */
void print_string(WINDOW *window, const char *string, int y, int x);

#endif