#ifdef TEST
#include "priv_line_editor.h"
#else
#include "line_editor.h"
#include "../../libs/src/priv_message.h"
#endif

#include "display.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#ifdef TEST
#include "../../libs/src/mock.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define COMMAND_HISTORY 10

#define CRLF_LEN 2
#define LEAD_CHAR_LEN 1

STATIC void display_command_text(LineEditor *lnEditor, RegMessage *message);

#ifndef TEST

/* a LnEditorCmd operates on the text and the
    cursor in the "input window". each line editor 
    command is mapped to a function that 
    performs the desired line editor action */

struct LnEditorCmd {
    int keyCode;
    LnEditorFunc lnEditorFunc;
};

/* a line editor is implemented as a circular
    buffer with a queue data structure. it stores
    text input and commands (input text is considered
    a command if it's passed to the command parser).
    the editor also tracks cursor position and 
    character count of the input text */
struct LineEditor {
    WINDOW *window;
    Queue *buffer;
    int cursor;
    int charCount;
};

#endif

static const LnEditorCmd LN_EDITOR_CMD[] = {
    {KEY_LEFT, move_cursor_left},
    {KEY_RIGHT, move_cursor_right},
    {KEY_UP, display_previous_command},
    {KEY_DOWN, display_next_command},
    {KEY_BACKSPACE, use_backspace},
    {KEY_DC, use_delete},
    {KEY_HOME, use_home},
    {KEY_END, use_end}
};

LineEditor * create_line_editor(WINDOW *window) {

    LineEditor *lnEditor = (LineEditor *) malloc(sizeof(LineEditor));
    if (lnEditor == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    lnEditor->buffer = create_queue(COMMAND_HISTORY, sizeof(RegMessage));
    lnEditor->window = window;
    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 0;

    return lnEditor;
}

void delete_line_editor(LineEditor *lnEditor) {

    if (lnEditor != NULL) {
        delete_queue(lnEditor->buffer);
    }
    free(lnEditor);
}

void move_cursor_left(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (lnEditor->cursor > PROMPT_SIZE) {

        wmove(lnEditor->window, 0, --lnEditor->cursor);
    }
}

void move_cursor_right(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (lnEditor->cursor - PROMPT_SIZE < lnEditor->charCount) {

        wmove(lnEditor->window, 0, ++lnEditor->cursor);
    }
}

void delete_char(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *regMessage = get_current_item(lnEditor->buffer);
    set_char_in_message(regMessage, '\0', lnEditor->cursor-PROMPT_SIZE, get_reg_message_content);
    --lnEditor->charCount;
}

void add_char(LineEditor *lnEditor, char ch) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    /* input is limited either by the column
        count or the constant MAX_CHARS,
        whichever number is lower */
    int cols = get_wwidth(lnEditor->window);

    if (lnEditor->cursor < cols && lnEditor->charCount + PROMPT_SIZE < cols && lnEditor->charCount < MAX_CHARS - CRLF_LEN - LEAD_CHAR_LEN) {

        /* depending on the cursor position, the 
            chars in the buffer may be moved to 
            accomodate an additonal char */
        char lastChPos = lnEditor->charCount + PROMPT_SIZE;

        for (int i = lastChPos; i > lnEditor->cursor; i--) {

            /* mvwinch retrieves char from the 
                window at position y, x */
            char shiftCh = mvwinch(lnEditor->window, 0, i - 1);
            mvwaddch(lnEditor->window, 0, i, shiftCh);

            /* RegMessage is a string container */
            RegMessage *regMessage = get_current_item(lnEditor->buffer);
            char ch = get_char_from_message(regMessage, i-PROMPT_SIZE-1, get_reg_message_content);
            set_char_in_message(regMessage, ch, i-PROMPT_SIZE, get_reg_message_content);

        }
        mvwaddch(lnEditor->window, 0, lnEditor->cursor, ch);   

        RegMessage *regMessage = get_current_item(lnEditor->buffer);
        set_char_in_message(regMessage, ch, lnEditor->cursor-PROMPT_SIZE, get_reg_message_content);

        lnEditor->charCount++;        
        lnEditor->cursor++;  

    }
}

void use_backspace(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (lnEditor->cursor > PROMPT_SIZE) {

        --lnEditor->cursor;
        delete_char(lnEditor);

        wmove(lnEditor->window, 0, lnEditor->cursor);
        wdelch(lnEditor->window);
    }
}

void use_delete(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (lnEditor->cursor - PROMPT_SIZE < lnEditor->charCount) {

        delete_char(lnEditor);
        wdelch(lnEditor->window);
    }
}

void use_home(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    wmove(lnEditor->window, 0, PROMPT_SIZE);

}

void use_end(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    wmove(lnEditor->window, 0, lnEditor->charCount + PROMPT_SIZE);
}

void display_current_command(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = get_current_item(lnEditor->buffer);
    display_command_text(lnEditor, message);
}

void display_previous_command(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = get_previous_item(lnEditor->buffer);
    display_command_text(lnEditor, message);
}

void display_next_command(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = get_next_item(lnEditor->buffer);
    display_command_text(lnEditor, message);
}

LnEditorFunc get_lneditor_function(int index) {

    LnEditorFunc lnEditorFunc = NULL;

    if (index >= 0 && index < ARR_SIZE(LN_EDITOR_CMD)) {

        lnEditorFunc = LN_EDITOR_CMD[index].lnEditorFunc;
    }
    return lnEditorFunc;
}

int get_le_func_index(int keyCode) {

    int cmdIndex = -1;

    for (int i = 0; i < ARR_SIZE(LN_EDITOR_CMD) && cmdIndex == -1; i++) {

        if (LN_EDITOR_CMD[i].keyCode == keyCode) {
            cmdIndex = i;
        }
    }
    return cmdIndex;
}

/* display command from the command queue */
STATIC void display_command_text(LineEditor *lnEditor, RegMessage *message) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (message != NULL) {

        char *content = get_reg_message_content(message);

        delete_part_line(lnEditor->window, PROMPT_SIZE);
        mvwprintw(lnEditor->window, 0, PROMPT_SIZE, content);
        
        lnEditor->charCount = strlen(content);
        lnEditor->cursor = PROMPT_SIZE + strlen(content);
        wrefresh(lnEditor->window);
    }
}

WINDOW * le_get_window(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return lnEditor->window;
}

Queue * le_get_buffer(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return lnEditor->buffer;
}

int le_get_char_count(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return lnEditor->charCount;
}

void le_set_char_count(LineEditor *lnEditor, int charCount) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    lnEditor->charCount = charCount;
}

int le_get_cursor(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return lnEditor->cursor;
}

void le_set_cursor(LineEditor *lnEditor, int cursor) {
    
    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    lnEditor->cursor = cursor;
}
