#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include "base_window.h"
#include "../../libs/src/queue.h"

#define PROMPT_SIZE 2

/*
 * Interface for managing user keyboard input in a line editor.
 *
 * This file contains the definitions and function declarations for a line editor
 * and an input window. The line editor tracks the character count, cursor position,
 * and stores entered commands for access through command history.
 *
 * The following editing functions are supported:
 * - BACKSPACE key deletes the previous character.
 * - DELETE key deletes the current character.
 * - HOME key moves the cursor to the start of the line.
 * - END key moves the cursor to the end of the text.
 * - LEFT ARROW key moves the cursor one position to the left.
 *
 */
typedef struct LineEditor LineEditor;

/* a container for the base window and line editor */
typedef struct InputWindow InputWindow;

InputWindow * create_input_window(int rows, int cols, int startY, int startX, int cmdHistoryCount);
void delete_input_window(InputWindow *inputWindow);

LineEditor * create_line_editor(int inputWidth, int cmdHistoryCount);
void delete_line_editor(LineEditor *lnEditor);

int move_le_cursor_left(LineEditor *lnEditor);
int move_le_cursor_right(LineEditor *lnEditor);

/* add a character to the line editor */
int add_le_char(LineEditor *lnEditor, char ch);

/* use backspace to delete the previous character 
from line editor */
int use_le_backspace(LineEditor *lnEditor);

/* use delete to delete the current character 
 * from line editor*/
int use_le_delete(LineEditor *lnEditor);

int use_le_home(LineEditor *lnEditor);
int use_le_end(LineEditor *lnEditor);

/* check if there is space available to add more characters */
bool can_add_char(LineEditor *lnEditor);

/* reload saved commands into the line editor */
void reload_saved_commands(LineEditor *lnEditor);

/* get the front command queue of the line editor */
Queue * get_le_front_cmd_queue(LineEditor *lnEditor);

/* get the back command queue of the line editor */
Queue * get_le_back_cmd_queue(LineEditor *lnEditor);

/* get/ set the character count in the line editor */
int get_le_char_count(LineEditor *lnEditor);
void set_le_char_count(LineEditor *lnEditor, int charCount);

int get_le_cursor(LineEditor *lnEditor);
void set_le_cursor(LineEditor *lnEditor, int cursor);

BaseWindow * get_le_base_window(InputWindow *inputWindow);
LineEditor * get_line_editor(InputWindow *inputWindow);

#endif