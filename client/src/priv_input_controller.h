/* --INTERNAL HEADER--
    used for testing */
#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include "priv_base_window.h"
#include "priv_scrollback_window.h"
#include "priv_input_window.h"

#define KEY_CTRLUP 1
#define KEY_CTRLDOWN 2

typedef void (*KeyboardCmdFunc)(BaseWindow *baseWindow);

typedef struct {
    int keyCode;
    KeyboardCmdFunc keyboardCmdFunc;
    WindowType windowType;
} KeyboardCmd;

KeyboardCmdFunc get_keyboard_cmd_function(int keyCode);
WindowType code_to_window_type(int keyCode);

int get_char(BaseWindow *baseWindow);
void add_char(BaseWindow *baseWindow, char ch);

void reload_scrollback(BaseWindow *baseWindow);

void display_current_command(BaseWindow *baseWindow);

int remap_ctrl_key(int ch);

#ifdef TEST

void scroll_line_up(BaseWindow *baseWindow);
void scroll_line_down(BaseWindow *baseWindow);
void scroll_page_up(BaseWindow *baseWindow);
void scroll_page_down(BaseWindow *baseWindow);

void move_cursor_left(BaseWindow *baseWindow);
void move_cursor_right(BaseWindow *baseWindow);
void use_backspace(BaseWindow *baseWindow);
void use_delete(BaseWindow *baseWindow);
void use_home(BaseWindow *baseWindow);
void use_end(BaseWindow *baseWindow);

void display_previous_command(BaseWindow *baseWindow);
void display_next_command(BaseWindow *baseWindow);

void display_command_text(WINDOW *window, LineEditor *lnEditor, const char *content);

#endif

#endif