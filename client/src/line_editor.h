#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#define NCURSES_WIDECHAR 1

#include "../../libs/src/queue.h"

#include <ncursesw/curses.h>

/* a map of line editor commands and callbacks */
typedef struct LnEditorCmd LnEditorCmd;

/* a line editor provides line editing functions. 
    
    the following functions are supported:
    - STANDARD key adds one char,
    - BACKSPACE deletes the previous char, 
    - DELETE deletes the current char,
    - HOME moves the cursor to the start of the line,
    - END moves the cursor to the end of the text,
    - LEFT ARROW moves the cursor one position to the left,
    - RIGHT ARROW moves the cursor one position to the right. 
    
    a command history is accessible with arrow keys */
typedef struct LineEditor LineEditor;

typedef void (*LnEditorFunc)(LineEditor *lnEditor);

LineEditor * create_line_editor(WINDOW *window);
void delete_line_editor(LineEditor *lnEditor);

/* below functions control the appearance of the input
    text and position of the cursor */
void move_cursor_left(LineEditor *lnEditor);
void move_cursor_right(LineEditor *lnEditor);
void delete_char(LineEditor *lnEditor);
void add_char(LineEditor *lnEditor, char ch);
void use_backspace(LineEditor *lnEditor);
void use_delete(LineEditor *lnEditor);
void use_home(LineEditor *lnEditor);
void use_end(LineEditor *lnEditor);

/* below functions provide access to the command history */
void display_current_command(LineEditor *lnEditor);
void display_previous_command(LineEditor *lnEditor);
void display_next_command(LineEditor *lnEditor);

LnEditorFunc get_lneditor_function(int index);
int get_le_func_index(int keyCode);

WINDOW * le_get_window(LineEditor *lnEditor);
Queue * le_get_cmd_queue(LineEditor *lnEditor);
int le_get_char_count(LineEditor *lnEditor);
void le_set_char_count(LineEditor *lnEditor, int charCount);
int le_get_cursor(LineEditor *lnEditor);
void le_set_cursor(LineEditor *lnEditor, int cursor);

#endif