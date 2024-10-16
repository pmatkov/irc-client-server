#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#define NCURSES_WIDECHAR 1

#include "../../libs/src/queue.h"

#include <ncursesw/curses.h>

/* represents a map of line editor commands 
    and functions */
typedef struct LnEditorCmd LnEditorCmd;

/* a line editor is used to edit the input text.  
    all the basic editing features are supported.
    
    the text can be edited in the following ways:
    - character key adds one char,
    - BACKSPACE deletes the previous char, 
    - DELETE deletes the current char,
    - HOME moves the cursor to the start of the line,
    - END moves the cursor to the end of the text,
    - LEFT ARROW moves the cursor one position to the left,
    - RIGHT ARROW moves the cursor one position to the right. 
    
    Additionally, a command history can be accessed
    with up and down arrows */
typedef struct LineEditor LineEditor;

typedef void (*LnEditorFunc)(LineEditor *lnEditor);

LineEditor * create_line_editor(WINDOW *window);
void delete_line_editor(LineEditor *lnEditor);

/* these functions operate on the input text and 
    change the cursor position on the screen */
void move_cursor_left(LineEditor *lnEditor);
void move_cursor_right(LineEditor *lnEditor);
void delete_char(LineEditor *lnEditor);
void add_char(LineEditor *lnEditor, char ch);
void use_backspace(LineEditor *lnEditor);
void use_delete(LineEditor *lnEditor);
void use_home(LineEditor *lnEditor);
void use_end(LineEditor *lnEditor);

/* these functions provide access to the command history */
void display_current_command(LineEditor *lnEditor);
void display_previous_command(LineEditor *lnEditor);
void display_next_command(LineEditor *lnEditor);

LnEditorFunc get_lneditor_function(int index);
int get_le_func_index(int keyCode);

WINDOW * le_get_window(LineEditor *lnEditor);
Queue * le_get_buffer(LineEditor *lnEditor);
int le_get_char_count(LineEditor *lnEditor);
void le_set_char_count(LineEditor *lnEditor, int charCount);
int le_get_cursor(LineEditor *lnEditor);
void le_set_cursor(LineEditor *lnEditor, int cursor);

#endif