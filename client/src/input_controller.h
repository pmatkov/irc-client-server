#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include "base_window.h"

/* a function pointer type for keyboard commands callbacks */
typedef void (*KeyboardCmdFunc)(BaseWindow *baseWindow);

typedef struct KeyboardCmd KeyboardCmd;

/* retrieve a command function based on the key code.
 * 
 * These functions execute control keys, for example, by moving a cursor
 * or changing a scrollback position.
 * 
 */
KeyboardCmdFunc get_keyboard_cmd_function(int keyCode);

WindowType code_to_window_type(int keyCode);

/* wrapper for ncurses wgetch function */
int get_char(BaseWindow *baseWindow);

/* add a character to the input window */
void add_char(BaseWindow *baseWindow, char ch);

/* reload scrollback on window resize */
void reload_scrollback(BaseWindow *baseWindow);

/* display the current command from the command history */
void display_current_command(BaseWindow *baseWindow);

/* translate ncurses CTRL + up or CTRL + down key identifiers to key constants */
int remap_ctrl_key(int ch);

#endif