#include "line_editor.h"
#include "command_processor.h"
#include "../../shared/src/queue.h"
#include "../../shared/src/parser.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 512
#define DEF_ARGS 4
#define MSG_HISTORY 10

struct LineEditor {
    WINDOW *window;
    MessageQueue *buffer;
    int cursor;
    int charCount;
};

// line editor stores user input to enable line editing
LineEditor * create_line_editor(WINDOW *window) {

    LineEditor *lnEditor = (LineEditor *) malloc(sizeof(LineEditor));
    if (lnEditor == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    lnEditor->buffer = create_message_queue(REGULAR_MSG, MSG_HISTORY);
    lnEditor->window = window;
    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 0;

    return lnEditor;
}

void delete_line_editor(LineEditor *lnEditor) {

    if (lnEditor != NULL) {
        delete_message_queue(lnEditor->buffer);
    }
    free(lnEditor);
}

MessageQueue * get_message_queue(LineEditor *lnEditor) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    
    return lnEditor->buffer;

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

    RegMessage *regMessage = get_message(lnEditor->buffer, 0);
    set_char_in_message(regMessage, '\0', lnEditor->cursor-PROMPT_SIZE);
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

            RegMessage *regMessage = get_message(lnEditor->buffer, 0);
            char ch = get_char_from_message(regMessage, i-PROMPT_SIZE-1);
            set_char_in_message(regMessage, ch, i-PROMPT_SIZE);

        }
        mvwaddch(lnEditor->window, 0, lnEditor->cursor, ch);  

        RegMessage *regMessage = get_message(lnEditor->buffer, 0);
        set_char_in_message(regMessage, ch, lnEditor->cursor-PROMPT_SIZE);

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

void display_command_history(LineEditor *lnEditor, int direction) {

    if (lnEditor == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *msg = get_message(lnEditor->buffer, direction);

    if (msg == NULL) {
        return;
    }

    char *content = get_message_content(msg);

    delete_part_line(lnEditor->window, PROMPT_SIZE);
    mvwprintw(lnEditor->window, 0, PROMPT_SIZE, content);
    
    lnEditor->charCount = strlen(content);
    lnEditor->cursor = PROMPT_SIZE + strlen(content);
    wrefresh(lnEditor->window);
}

// parse command line input
void parse_input(LineEditor *lnEditor, Scrollback *scrollback, Settings *settings, Session *session) {

    if (lnEditor == NULL || session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (lnEditor->charCount <= 0 && lnEditor->charCount > MAX_CHARS) {
        return;
    }

    RegMessage *msg = get_message(lnEditor->buffer, 0);
    set_char_in_message(msg, '\0', lnEditor->charCount);

    char *input = get_message_content(msg);

    if (has_command_prefix(input)) {

        // split input to tokens
        char *inputCopy = strdup(input);
        char *tokens[DEF_ARGS] = {NULL};

        int tkCount = split_input_string(inputCopy, tokens, DEF_ARGS, ' ');

        CommandType commandType = string_to_command_type(tokens[0]);
  
        get_command_function(commandType)(scrollback, settings, session, tokens, tkCount);

        free(inputCopy);
    }
    else if (session_is_inchannel(session)) {

         get_command_function(MSG)(scrollback, settings, session, &input, 1);
    }

    delete_part_line(lnEditor->window, PROMPT_SIZE);

    lnEditor->charCount = 0;
    lnEditor->cursor = PROMPT_SIZE;

    wrefresh(lnEditor->window);

    RegMessage newMsg;
    set_reg_message(&newMsg, input);
    enqueue(lnEditor->buffer, &newMsg);

}