#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include "base_window.h"
#include "../../libs/src/queue.h"

#define PROMPT_SIZE 2

/**
 * @struct LineEditor
 * @brief Interface for managing user keyboard input in a line editor.
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

/**
 * @struct InputWindow
 * @brief A container for the base window and line editor.
 */
typedef struct InputWindow InputWindow;

/**
 * @brief Create an input window.
 * 
 * @param rows Number of rows for the window.
 * @param cols Number of columns for the window.
 * @param startY Starting Y position for the window.
 * @param startX Starting X position for the window.
 * @param cmdHistoryCount Number of commands to store in history.
 * @return Pointer to the created InputWindow.
 */
InputWindow * create_input_window(int rows, int cols, int startY, int startX, int cmdHistoryCount);

/**
 * @brief Delete an input window.
 * 
 * @param inputWindow Pointer to the InputWindow to delete.
 */
void delete_input_window(InputWindow *inputWindow);

/**
 * @brief Create a line editor.
 * 
 * @param inputWidth Width of the input area.
 * @param cmdHistoryCount Number of commands to store in history.
 * @return Pointer to the created LineEditor.
 */
LineEditor * create_line_editor(int inputWidth, int cmdHistoryCount);

/**
 * @brief Delete a line editor.
 * 
 * @param lnEditor Pointer to the LineEditor to delete.
 */
void delete_line_editor(LineEditor *lnEditor);

/**
 * @brief Move the cursor to the left.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Status code (0 for success, non-zero for failure).
 */
int move_le_cursor_left(LineEditor *lnEditor);

/**
 * @brief Move the cursor to the right.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Status code (0 for success, non-zero for failure).
 */
int move_le_cursor_right(LineEditor *lnEditor);

/**
 * @brief Add a character to the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @param ch Character to add.
 * @return Status code (0 for success, non-zero for failure).
 */
int add_le_char(LineEditor *lnEditor, char ch);

/**
 * @brief Use backspace to delete the previous character.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Status code (0 for success, non-zero for failure).
 */
int use_le_backspace(LineEditor *lnEditor);

/**
 * @brief Use delete to delete the current character.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Status code (0 for success, non-zero for failure).
 */
int use_le_delete(LineEditor *lnEditor);

/**
 * @brief Move the cursor to the start of the line.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Status code (0 for success, non-zero for failure).
 */
int use_le_home(LineEditor *lnEditor);

/**
 * @brief Move the cursor to the end of the line.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Status code (0 for success, non-zero for failure).
 */
int use_le_end(LineEditor *lnEditor);

/**
 * @brief Check if there is space available to add more characters.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return True if space is available, false otherwise.
 */
bool can_add_char(LineEditor *lnEditor);

/**
 * @brief Reload saved commands into the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 */
void reload_saved_commands(LineEditor *lnEditor);

/**
 * @brief Get the front command queue of the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Pointer to the front command queue.
 */
Queue * get_le_front_cmd_queue(LineEditor *lnEditor);

/**
 * @brief Get the back command queue of the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Pointer to the back command queue.
 */
Queue * get_le_back_cmd_queue(LineEditor *lnEditor);

/**
 * @brief Get the character count in the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Character count.
 */
int get_le_char_count(LineEditor *lnEditor);

/**
 * @brief Set the character count in the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @param charCount Character count to set.
 */
void set_le_char_count(LineEditor *lnEditor, int charCount);

/**
 * @brief Get the cursor position in the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @return Cursor position.
 */
int get_le_cursor(LineEditor *lnEditor);

/**
 * @brief Set the cursor position in the line editor.
 * 
 * @param lnEditor Pointer to the LineEditor.
 * @param cursor Cursor position to set.
 */
void set_le_cursor(LineEditor *lnEditor, int cursor);

/**
 * @brief Get the base window from the input window.
 * 
 * @param inputWindow Pointer to the InputWindow.
 * @return Pointer to the BaseWindow.
 */
BaseWindow * get_le_base_window(InputWindow *inputWindow);

/**
 * @brief Get the line editor from the input window.
 * 
 * @param inputWindow Pointer to the InputWindow.
 * @return Pointer to the LineEditor.
 */
LineEditor * get_line_editor(InputWindow *inputWindow);

#endif