#ifndef SCROLLBACK_WINDOW_H
#define SCROLLBACK_WINDOW_H

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include "scroll_subject.h"
#include "base_window.h"

#include <ncursesw/curses.h>

/**
 * @struct Scrollback
 * @brief Interface for storing off-screen text with a predefined capacity.
 *    Additional entries will overwrite the oldest entries in the buffer.
 *
*    The text can be scrolled with the following keys:
*    - CTRL + UP ARROW: Scrolls one row up
*    - CTRL + DOWN ARROW: Scrolls one row down
*    - PAGE UP: Scrolls one screen up
*    - PAGE DOWN: Scrolls one screen down
 */
typedef struct Scrollback Scrollback;

/**
 * @struct ScrollbackWindow
 * @brief A container for base window and scrollback.
 */
typedef struct ScrollbackWindow ScrollbackWindow;

/**
 * @brief Creates a scrollback window.
 * 
 * @param rows Number of rows in the window.
 * @param cols Number of columns in the window.
 * @param startY Starting Y position of the window.
 * @param startX Starting X position of the window.
 * @param multiplier Multiplier for the scrollback buffer.
 * @return Pointer to the created ScrollbackWindow.
 */
ScrollbackWindow * create_scrollback_window(int rows, int cols, int startY, int startX, int multiplier);

/**
 * @brief Deletes a scrollback window.
 * 
 * @param scrollbackWindow Pointer to the ScrollbackWindow to delete.
 */
void delete_scrollback_window(ScrollbackWindow *scrollbackWindow);

/**
 * @brief Creates a scrollback buffer.
 * 
 * @param viewportHeight Height of the viewport.
 * @param multiplier Multiplier for the scrollback buffer.
 * @return Pointer to the created Scrollback.
 */
Scrollback * create_scrollback(int viewportHeight, int multiplier);

/**
 * @brief Deletes a scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback to delete.
 */
void delete_scrollback(Scrollback *scrollback);

/**
 * @brief Checks if the scrollback buffer is empty.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return true if the scrollback buffer is empty, false otherwise.
 */
bool is_scrollback_empty(Scrollback *scrollback);

/**
 * @brief Checks if the scrollback buffer is full.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return true if the scrollback buffer is full, false otherwise.
 */
bool is_scrollback_full(Scrollback *scrollback);

/**
 * @brief Adds one line of text to the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param string Line of text to add.
 */
void add_to_scrollback(Scrollback *scrollback, cchar_t *string);

/**
 * @brief Moves the visible scrollback position up.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param count Number of lines to move up.
 */
void move_sb_up(Scrollback *scrollback, int count);

/**
 * @brief Moves the visible scrollback position down.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param count Number of lines to move down.
 */
void move_sb_down(Scrollback *scrollback, int count);

/**
 * @brief Calculates the remaining lines at the top end of the buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Number of remaining lines at the top end.
 */
int count_remaining_top_lines(Scrollback *scrollback);

/**
 * @brief Calculates the remaining lines at the bottom end of the buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Number of remaining lines at the bottom end.
 */
int count_remaining_bottom_lines(Scrollback *scrollback);

/**
 * @brief Gets the head position of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Head position of the scrollback buffer.
 */
int get_sb_head(Scrollback *scrollback);

/**
 * @brief Gets the top line position of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Top line position of the scrollback buffer.
 */
int get_sb_topline(Scrollback *scrollback);

/**
 * @brief Sets the top line position of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param topLine Top line position to set.
 */
void set_sb_topline(Scrollback *scrollback, int topLine);

/**
 * @brief Gets the bottom line position of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Bottom line position of the scrollback buffer.
 */
int get_sb_bottomline(Scrollback *scrollback);

/**
 * @brief Sets the bottom line position of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param bottomLine Bottom line position to set.
 */
void set_sb_bottomline(Scrollback *scrollback, int bottomLine);

/**
 * @brief Gets the capacity of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Capacity of the scrollback buffer.
 */
int get_sb_capacity(Scrollback *scrollback);

/**
 * @brief Checks if the scrollback buffer is scrolled up.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return true if the scrollback buffer is scrolled up, false otherwise.
 */
bool is_sb_scrolled_up(Scrollback *scrollback);

/**
 * @brief Sets the scroll state of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param isScrolledUp Scroll state to set.
 */
void set_sb_is_scrolled_up(Scrollback *scrollback, bool isScrolledUp);

/**
 * @brief Gets the scroll subject of the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @return Pointer to the ScrollSubject.
 */
ScrollSubject * get_scroll_subject(Scrollback *scrollback);

/**
 * @brief Gets a line from the scrollback buffer.
 * 
 * @param scrollback Pointer to the Scrollback.
 * @param index Index of the line to get.
 * @return Pointer to the line of text.
 */
cchar_t * get_sb_buffer_line(Scrollback *scrollback, int index);

/**
 * @brief Gets the scrollback buffer from the scrollback window.
 * 
 * @param scrollbackWindow Pointer to the ScrollbackWindow.
 * @return Pointer to the Scrollback.
 */
Scrollback * get_scrollback(ScrollbackWindow *scrollbackWindow);

/**
 * @brief Gets the base window from the scrollback window.
 * 
 * @param scrollbackWindow Pointer to the ScrollbackWindow.
 * @return Pointer to the BaseWindow.
 */
BaseWindow * get_sb_base_window(ScrollbackWindow *scrollbackWindow);

#endif