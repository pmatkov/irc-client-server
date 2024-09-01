#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#include "../src/scrollback.h"
#include "../src/settings.h"
#include "../src/session.h"

#include <ncursesw/curses.h>

typedef struct {
    WINDOW *window;
    char *buffer;
    int cursor;
    int charCount;
} LineEditor;

LineEditor * create_line_editor(WINDOW *window);
void delete_line_editor(LineEditor *lnEditor);

MessageQueue * get_message_queue(LineEditor *lnEditor);

void move_cursor_left(LineEditor *lnEditor);
void move_cursor_right(LineEditor *lnEditor);
void delete_char(LineEditor *lnEditor);
void add_char(LineEditor *lnEditor, char ch);
void use_backspace(LineEditor *lnEditor);
void use_delete(LineEditor *lnEditor);
void use_home(LineEditor *lnEditor);
void use_end(LineEditor *lnEditor);

void display_command_history(LineEditor *lnEditor, int direction);

void parse_input(LineEditor *lnEditor, Scrollback *scrollback, Settings *settings, Session *session);

#endif