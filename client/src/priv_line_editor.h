/* --INTERNAL HEADER--
    used for unit testing */

#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#define NCURSES_WIDECHAR 1

#include "../../shared/src/priv_queue.h"
#include "../../shared/src/priv_message.h"

#include <ncursesw/curses.h>

typedef struct {
    WINDOW *window;
    Queue *buffer;
    int cursor;
    int charCount;
} LineEditor;

typedef void (*LnEditorFunction)(LineEditor *lnEditor);

typedef struct {
    int keyCode;
    LnEditorFunction lnEditorFunc;
} LnEditorCmd;

LineEditor * create_line_editor(WINDOW *window);
void delete_line_editor(LineEditor *lnEditor);

void move_cursor_left(LineEditor *lnEditor);
void move_cursor_right(LineEditor *lnEditor);
void delete_char(LineEditor *lnEditor);
void add_char(LineEditor *lnEditor, char ch);
void use_backspace(LineEditor *lnEditor);
void use_delete(LineEditor *lnEditor);
void use_home(LineEditor *lnEditor);
void use_end(LineEditor *lnEditor);

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

#ifdef TEST

void display_command_text(LineEditor *lnEditor, RegMessage *msg);

#endif

#endif