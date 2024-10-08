#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#define NCURSES_WIDECHAR 1

#include "../../shared/src/queue.h"

#include <ncursesw/curses.h>

/* LnEditorCmd represents a map of line editor commands 
    and functions */
typedef struct LnEditorCmd LnEditorCmd;

/* Line editor enables editing of user input text.  
    basic editing functionalities are supported.
    
    the text can be edited in the following ways:
    - add one char with standard character key,
    - delete previous char with BACKSPACE key, 
    - delete current char with DELETE key,
    - move cursor to the start of the line with HOME key,
    - move cursor to the end of the line with END key,
    - move cursor one char to the left with LEFT ARROW key,
    - move cursor one char to the right with RIGHT ARROW  key. 
    
    Also supported is command history functionality which
    can be accessed with up and down arrow keys. */
typedef struct LineEditor LineEditor;

typedef void (*LnEditorFunction)(LineEditor *lnEditor);

LineEditor * create_line_editor(WINDOW *window);
void delete_line_editor(LineEditor *lnEditor);

/* below functions mainuplate input text and cursor
    position on screen */
void move_cursor_left(LineEditor *lnEditor);
void move_cursor_right(LineEditor *lnEditor);
void delete_char(LineEditor *lnEditor);
void add_char(LineEditor *lnEditor, char ch);
void use_backspace(LineEditor *lnEditor);
void use_delete(LineEditor *lnEditor);
void use_home(LineEditor *lnEditor);
void use_end(LineEditor *lnEditor);

/* below functions provide access to command history */
void display_current_command(LineEditor *lnEditor);
void display_previous_command(LineEditor *lnEditor);
void display_next_command(LineEditor *lnEditor);

LnEditorFunction get_lneditor_function(int index);
int get_le_func_index(int keyCode);

WINDOW * le_get_window(LineEditor *lnEditor);
Queue * le_get_buffer(LineEditor *lnEditor);
int le_get_char_count(LineEditor *lnEditor);
void le_set_char_count(LineEditor *lnEditor, int charCount);
int le_get_cursor(LineEditor *lnEditor);
void le_set_cursor(LineEditor *lnEditor, int cursor);

#endif