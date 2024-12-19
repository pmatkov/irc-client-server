#ifdef TEST
#include "priv_input_window.h"
#include "../../libs/src/mock.h"
#else
#include "input_window.h"
#include "priv_base_window.h"
#include "../../libs/src/common.h"
#endif

#include "../../libs/src/message.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct LineEditor {
    Queue *frontCmdQueue;
    Queue *backCmdQueue;
    int inputWidth;
    int cursor;
    int charCount;
};

struct InputWindow {
    BaseWindow baseWindow;
    LineEditor *lineEditor;
};

#endif

#define DEF_HISTORY 10

STATIC void delete_le_char(LineEditor *lnEditor);

InputWindow * create_input_window(int rows, int cols, int startY, int startX, int cmdHistoryCount) {

    InputWindow *inputWindow = (InputWindow *) malloc(sizeof(InputWindow));
    if (inputWindow == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }
    inputWindow->baseWindow.window = newwin(rows, cols, startY, startX);
    inputWindow->baseWindow.rows = rows;
    inputWindow->baseWindow.cols = cols;
    inputWindow->baseWindow.windowType = INPUT_WINDOW;
    inputWindow->lineEditor = create_line_editor(cols, cmdHistoryCount);

    return inputWindow;
}

void delete_input_window(InputWindow *inputWindow) {

    if (inputWindow != NULL) {

        delwin(inputWindow->baseWindow.window);
        if (inputWindow->lineEditor != NULL) {
            delete_line_editor(inputWindow->lineEditor);
        }
    }
    free(inputWindow);
}

LineEditor * create_line_editor(int inputWidth, int cmdHistoryCount) {

    if (cmdHistoryCount < 1) {
        cmdHistoryCount = DEF_HISTORY;
    }

    LineEditor *lnEditor = (LineEditor *) malloc(sizeof(LineEditor));
    if (lnEditor == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    lnEditor->frontCmdQueue = create_queue(cmdHistoryCount, get_message_size());
    lnEditor->backCmdQueue = create_queue(cmdHistoryCount, get_message_size());
    lnEditor->inputWidth = inputWidth;
    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 0;

    return lnEditor;
}

void delete_line_editor(LineEditor *lnEditor) {

    if (lnEditor != NULL) {
        delete_queue(lnEditor->frontCmdQueue);
        delete_queue(lnEditor->backCmdQueue);
    }
    free(lnEditor);
}

int move_le_cursor_left(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int moved = 0;

    if (lnEditor->cursor > PROMPT_SIZE) {

        lnEditor->cursor--;
        moved = 1;
    }
    return moved;
}

int move_le_cursor_right(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int moved = 0;

    if (lnEditor->cursor - PROMPT_SIZE < lnEditor->charCount) {

        lnEditor->cursor++;
        moved = 1;
    }
    return moved;
}

int add_le_char(LineEditor *lnEditor, char ch) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int added = 0;
    
    if (can_add_char(lnEditor)) {

        int lastChIdx = lnEditor->charCount + PROMPT_SIZE;

        Message *frontCmd = get_current_item(lnEditor->frontCmdQueue);
        const char *frontContent = get_message_content(frontCmd);

        if (!strlen(frontContent)) {
            set_message_type(frontCmd, MSG_COMMAND);
            set_message_priority(frontCmd, NO_PRIORITY);
        }

        for (int i = lastChIdx; i > lnEditor->cursor; i--) {
            set_message_char(frontCmd, frontContent[i - PROMPT_SIZE - 1], i - PROMPT_SIZE);
        } 
        set_message_char(frontCmd, ch, lnEditor->cursor - PROMPT_SIZE);

        lnEditor->charCount++;        
        lnEditor->cursor++;  

        added = 1;
    }

    return added;
}

STATIC void delete_le_char(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Message *frontCmd = get_current_item(lnEditor->frontCmdQueue);

    if (frontCmd != NULL) {

        char buffer[MAX_CHARS + 1] = {'\0'};

        const char *frontContent = get_message_content(frontCmd);
        safe_copy(buffer, ARRAY_SIZE(buffer), frontContent);
        
        shift_chars(buffer, strlen(buffer), lnEditor->cursor - PROMPT_SIZE, lnEditor->cursor - PROMPT_SIZE + 1);
        set_message_content(frontCmd, buffer);
        --lnEditor->charCount;
    }
}

int use_le_backspace(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int deleted = 0;

    if (lnEditor->cursor > PROMPT_SIZE) {

        lnEditor->cursor--;
        delete_le_char(lnEditor);
        deleted = 1;

    }
    return deleted;
}

int use_le_delete(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int deleted = 0;

    if (lnEditor->cursor - PROMPT_SIZE < lnEditor->charCount) {

        delete_le_char(lnEditor);
        deleted = 1;
    }
    return deleted;
}

int use_le_home(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int moved = 0;

    if (lnEditor->cursor > PROMPT_SIZE) {

        lnEditor->cursor = PROMPT_SIZE;
        moved = 1;
    }
    return moved;
}

int use_le_end(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int moved = 0;

    if (lnEditor->cursor < lnEditor->charCount + PROMPT_SIZE) {
        lnEditor->cursor = lnEditor->charCount + PROMPT_SIZE;
        moved = 1;
    }
    return moved;
}

bool can_add_char(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    bool add = 0;

    if (lnEditor->cursor < lnEditor->inputWidth - 1 && lnEditor->charCount < MAX_CHARS)  {
        add = 1;
    }

    return add;
}

void reload_saved_commands(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    for (int i = 0; i < get_queue_capacity(lnEditor->frontCmdQueue); i++) {

        Message *message = get_item_at_idx(lnEditor->frontCmdQueue, i);
        const char *frontContent = get_message_content(message);
        const char *backContent = get_message_content(get_item_at_idx(lnEditor->backCmdQueue, i));

        if (strcmp(frontContent, backContent) != 0) {
            set_message_content(message, backContent);
        }
    }
}

Queue * get_le_front_cmd_queue(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return lnEditor->frontCmdQueue;
}

Queue * get_le_back_cmd_queue(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return lnEditor->backCmdQueue;
}

int get_le_char_count(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return lnEditor->charCount;
}

void set_le_char_count(LineEditor *lnEditor, int charCount) {

    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    lnEditor->charCount = charCount;
}

int get_le_cursor(LineEditor *lnEditor) {
    
    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return lnEditor->cursor;
}

void set_le_cursor(LineEditor *lnEditor, int cursor) {
    
    if (lnEditor == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    lnEditor->cursor = cursor;
}

BaseWindow * get_le_base_window(InputWindow *inputWindow) {

    if (inputWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return &inputWindow->baseWindow;
}

LineEditor * get_line_editor(InputWindow *inputWindow) {

    if (inputWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return inputWindow->lineEditor;
}

