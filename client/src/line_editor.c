#ifdef TEST
#include "priv_line_editor.h"
#else
#include "line_editor.h"
#include "../../shared/src/priv_message.h"
#endif

#include "display.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_CHARS 512
#define MSG_HISTORY 10

STATIC void display_command_text(LineEditor *lnEditor, RegMessage *msg);

#ifndef TEST

struct LnEditorCmd {
    int keyCode;
    LnEditorFunc lnEditorFunc;
};

struct LineEditor {
    WINDOW *window;
    Queue *buffer;
    int cursor;
    int charCount;
};

#endif

static const LnEditorCmd lnEditorCmd[] = {
    {KEY_LEFT, move_cursor_left},
    {KEY_RIGHT, move_cursor_right},
    {KEY_UP, display_previous_command},
    {KEY_DOWN, display_next_command},
    {KEY_BACKSPACE, use_backspace},
    {KEY_DC, use_delete},
    {KEY_HOME, use_home},
    {KEY_END, use_end}
};

// line editor stores user input to enable line editing
LineEditor * create_line_editor(WINDOW *window) {

    LineEditor *lnEditor = (LineEditor *) malloc(sizeof(LineEditor));
    if (lnEditor == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    lnEditor->buffer = create_queue(MSG_HISTORY, sizeof(RegMessage));
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

    int cols = get_wwidth(lnEditor->window);

    if (lnEditor->cursor < cols && lnEditor->charCount + PROMPT_SIZE < cols && lnEditor->charCount < MAX_CHARS) {

        char lastChPos = lnEditor->charCount + PROMPT_SIZE;

        for (int i = lastChPos; i > lnEditor->cursor; i--) {

            char shiftCh = mvwinch(lnEditor->window, 0, i-1);
            mvwaddch(lnEditor->window, 0, i, shiftCh);

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

void display_previous_command(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *msg = get_previous_item(lnEditor->buffer);
    display_command_text(lnEditor, msg);

}

void display_next_command(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *msg = get_next_item(lnEditor->buffer);
    display_command_text(lnEditor, msg);
}

LnEditorFunc use_line_editor_func(int index) {

    return lnEditorCmd[index].lnEditorFunc;
}

int get_le_func_index(int keyCode) {

    int cmdIndex = -1;

    for (int i = 0; i < sizeof(lnEditorCmd)/ sizeof(lnEditorCmd[0]) && cmdIndex == -1; i++) {

        if (lnEditorCmd[i].keyCode == keyCode) {
            cmdIndex = i;
        }
    }
    return cmdIndex;
}

STATIC void display_command_text(LineEditor *lnEditor, RegMessage *msg) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (msg != NULL) {

        char *content = get_reg_message_content(msg);

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
