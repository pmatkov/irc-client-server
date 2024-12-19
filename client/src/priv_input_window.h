/* --INTERNAL HEADER--
    used for testing */
#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include "priv_base_window.h"
#include "../../libs/src/priv_queue.h"
#include "../../libs/src/common.h"

#define PROMPT_SIZE 2

typedef struct {
    Queue *frontCmdQueue;
    Queue *backCmdQueue;
    int inputWidth;
    int cursor;
    int charCount;
} LineEditor;

typedef struct {
    BaseWindow baseWindow;
    LineEditor *lineEditor;
} InputWindow;

InputWindow * create_input_window(int rows, int cols, int startY, int startX, int cmdHistoryCount);
void delete_input_window(InputWindow *inputWindow);

LineEditor * create_line_editor(int inputWidth, int cmdHistoryCount);
void delete_line_editor(LineEditor *lnEditor);

int move_le_cursor_left(LineEditor *lnEditor);
int move_le_cursor_right(LineEditor *lnEditor);

int add_le_char(LineEditor *lnEditor, char ch);
int use_le_backspace(LineEditor *lnEditor);
int use_le_delete(LineEditor *lnEditor);

int use_le_home(LineEditor *lnEditor);
int use_le_end(LineEditor *lnEditor);

bool can_add_char(LineEditor *lnEditor);

void reload_saved_commands(LineEditor *lnEditor);

Queue * get_le_front_cmd_queue(LineEditor *lnEditor);
Queue * get_le_back_cmd_queue(LineEditor *lnEditor);
int get_le_char_count(LineEditor *lnEditor);
void set_le_char_count(LineEditor *lnEditor, int charCount);
int get_le_cursor(LineEditor *lnEditor);
void set_le_cursor(LineEditor *lnEditor, int cursor);

BaseWindow * get_le_base_window(InputWindow *inputWindow);
LineEditor * get_line_editor(InputWindow *inputWindow);

#ifdef TEST 

void delete_le_char(LineEditor *lnEditor);

#endif

#endif