#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include "base_window.h"

/**
 * @brief Typedef for a function pointer that takes a BaseWindow pointer as an argument.
 */
typedef void (*KeyboardCmdFunc)(BaseWindow *baseWindow);

/**
 * @brief Forward declaration of the KeyboardCmd struct.
 */
typedef struct KeyboardCmd KeyboardCmd;

/**
 * @brief Retrieve a command function based on the key code.
 * 
 * These functions execute control keys, for example, by moving a cursor
 * or changing a scrollback position.
 * 
 * @param keyCode The key code for which to retrieve the command function.
 * @return KeyboardCmdFunc The function pointer corresponding to the key code.
 */
KeyboardCmdFunc get_keyboard_cmd_function(int keyCode);

/**
 * @brief Convert a key code to a window type.
 * 
 * @param keyCode The key code to convert.
 * @return WindowType The corresponding window type.
 */
WindowType code_to_window_type(int keyCode);

/**
 * @brief Wrapper for ncurses wgetch function.
 * 
 * @param baseWindow The base window from which to get the character.
 * @return int The character code retrieved.
 */
int get_char(BaseWindow *baseWindow);

/**
 * @brief Add a character to the input window.
 * 
 * @param baseWindow The base window to which the character will be added.
 * @param ch The character to add.
 */
void add_char(BaseWindow *baseWindow, char ch);

/**
 * @brief Reload scrollback on window resize.
 * 
 * @param baseWindow The base window for which to restore the scrollback.
 */
void restore_from_scrollback(BaseWindow *baseWindow);

/**
 * @brief Display the current command from the command history.
 * 
 * @param baseWindow The base window in which to display the current command.
 */
void display_current_command(BaseWindow *baseWindow);

/**
 * @brief Translate ncurses CTRL + up or CTRL + down key identifiers to key constants.
 * 
 * @param ch The character code to remap.
 * @return int The remapped key constant.
 */
int remap_ctrl_key(int ch);

#endif