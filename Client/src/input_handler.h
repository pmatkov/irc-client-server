#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "scrollback.h"

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

typedef struct {
    WINDOW *window;
    char *buffer;
    int cursor;
    int charCount;
} LineEditor;

LineEditor * create_line_editor(WINDOW *window);
void delete_line_editor(LineEditor *lnEditor);

#ifdef TEST
STATIC void scroll_line_up(Scrollback *scrollback);
STATIC void scroll_line_down(Scrollback *scrollback);
STATIC void scroll_page_up(Scrollback *scrollback);
STATIC void scroll_page_down(Scrollback *scrollback);

STATIC void move_cursor_left(LineEditor *lnEditor);
STATIC void move_cursor_right(LineEditor *lnEditor);

STATIC void delete_char(LineEditor *lnEditor);
STATIC void add_char(LineEditor *lnEditor, char ch);
STATIC void use_backspace(LineEditor *lnEditor);
STATIC void use_delete(LineEditor *lnEditor);
#endif

#endif