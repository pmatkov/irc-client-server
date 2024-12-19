#ifdef TEST
#include "priv_input_controller.h"
#else
#include "input_controller.h"
#include "scrollback_window.h"
#include "input_window.h"
#endif

#include "print_manager.h"
#include "../../libs/src/message.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

#define KEY_CTRLUP 1
#define KEY_CTRLDOWN 2

/* a type which holds key codes for control 
    keys, functions which execute those controls
    and windows which implement those functions */

struct KeyboardCmd {
    int keyCode;
    KeyboardCmdFunc keyboardCmdFunc;
    WindowType windowType;
};

#endif

/* the command functions below only update the display
    on the screen. changes to the underlying buffer 
    (such as in the line editor or scrollback) are 
    managed directly by their respective window types */
STATIC void scroll_line_up(BaseWindow *baseWindow);
STATIC void scroll_line_down(BaseWindow *baseWindow);
STATIC void scroll_page_up(BaseWindow *baseWindow);
STATIC void scroll_page_down(BaseWindow *baseWindow);

STATIC void move_cursor_left(BaseWindow *baseWindow);
STATIC void move_cursor_right(BaseWindow *baseWindow);
STATIC void use_backspace(BaseWindow *baseWindow);
STATIC void use_delete(BaseWindow *baseWindow);
STATIC void use_home(BaseWindow *baseWindow);
STATIC void use_end(BaseWindow *baseWindow);

STATIC void display_previous_command(BaseWindow *baseWindow);
STATIC void display_next_command(BaseWindow *baseWindow);

STATIC void display_command_text(WINDOW *window, LineEditor *lnEditor, const char *content);

static const KeyboardCmd KEYBOARD_CMD[] = {
    {KEY_CTRLUP, scroll_line_up, SCROLLBACK_WINDOW},
    {KEY_CTRLDOWN, scroll_line_down, SCROLLBACK_WINDOW},
    {KEY_PPAGE, scroll_page_up, SCROLLBACK_WINDOW},
    {KEY_NPAGE, scroll_page_down, SCROLLBACK_WINDOW},
    {KEY_LEFT, move_cursor_left, INPUT_WINDOW},
    {KEY_RIGHT, move_cursor_right, INPUT_WINDOW},
    {KEY_BACKSPACE, use_backspace, INPUT_WINDOW},
    {KEY_DC, use_delete, INPUT_WINDOW},
    {KEY_HOME, use_home, INPUT_WINDOW},
    {KEY_END, use_end, INPUT_WINDOW},
    {KEY_UP, display_previous_command, INPUT_WINDOW},
    {KEY_DOWN, display_next_command, INPUT_WINDOW}
};

KeyboardCmdFunc get_keyboard_cmd_function(int keyCode) {

    KeyboardCmdFunc keyboardCmdFunc = NULL;

    for (int i = 0; i < ARRAY_SIZE(KEYBOARD_CMD) && keyboardCmdFunc == NULL; i++) {

        if (KEYBOARD_CMD[i].keyCode == keyCode) {
            keyboardCmdFunc = KEYBOARD_CMD[i].keyboardCmdFunc;
        }
    }
    return keyboardCmdFunc;
}

WindowType code_to_window_type(int keyCode) {

    WindowType windowType = UNKNOWN_WINDOW_TYPE;

    for (int i = 0; i < ARRAY_SIZE(KEYBOARD_CMD) && windowType == UNKNOWN_WINDOW_TYPE; i++) {

        if (KEYBOARD_CMD[i].keyCode == keyCode) {
            windowType = KEYBOARD_CMD[i].windowType;
        }
    }
    return windowType;
}

int get_char(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int ch = -1;

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
    }
    else {
        ch = wgetch(get_window(baseWindow));
    }

    return ch;
}

void add_char(BaseWindow *baseWindow, char ch) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    int cursor = get_le_cursor(lnEditor);
    int charCount = get_le_char_count(lnEditor);

    if (add_le_char(lnEditor, ch)) {

        int lastChIdx = charCount + PROMPT_SIZE;

        for (int i = lastChIdx; i > cursor; i--) {

            char currentCh = mvwinch(window, 0, i - 1);
            mvwaddch(window, 0, i, currentCh);
        }
        mvwaddch(window, 0, cursor, ch);
    }
}

void restore_from_scrollback(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != SCROLLBACK_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    Scrollback *scrollback = get_scrollback(((ScrollbackWindow*)baseWindow));

    int rows = get_window_rows(baseWindow);

    if (rows) {

        wmove(window, 0, 0);

        for (int i = get_sb_topline(scrollback), j = 0; i < rows && i < get_sb_head(scrollback); i++, j++) {

            print_cx_string(window, get_sb_buffer_line(scrollback, i), j, 0);
            set_sb_bottomline(scrollback, (i + 1) % get_sb_capacity(scrollback));
        }
        wrefresh(window);
    }
}

STATIC void scroll_line_up(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != SCROLLBACK_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    Scrollback *scrollback = get_scrollback(((ScrollbackWindow*)baseWindow));

    int topLines = count_remaining_top_lines(scrollback);

    if (topLines > 0) {
        wscrl(window, -1);
        curs_set(0);
        print_cx_string(window, get_sb_buffer_line(scrollback, get_sb_topline(scrollback) - 1), 0, 0);
        curs_set(1);
        wrefresh(window);

        move_sb_up(scrollback, 1);
    }
}

STATIC void scroll_line_down(BaseWindow *baseWindow) {
    
    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != SCROLLBACK_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    Scrollback *scrollback = get_scrollback(((ScrollbackWindow*)baseWindow));

    int rows = get_window_rows(baseWindow);
    int bottomLines = count_remaining_bottom_lines(scrollback);

    if (bottomLines > 0) {

        wscrl(window, 1);
        curs_set(0);
        print_cx_string(window, get_sb_buffer_line(scrollback, get_sb_bottomline(scrollback)),  rows - 1, 0);
        curs_set(1);
        wrefresh(window);

        move_sb_down(scrollback, 1);
    }  
}

STATIC void scroll_page_up(BaseWindow *baseWindow) {
    
    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != SCROLLBACK_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    Scrollback *scrollback = get_scrollback(((ScrollbackWindow*)baseWindow));

    int rows = get_window_rows(baseWindow);
    int topLines = count_remaining_top_lines(scrollback);

    if (topLines > 0) {

        int shift = topLines >= rows ? rows : topLines;
        wscrl(window, -shift);

        for (int i = 0; i < shift; i++) {

            print_cx_string(window, get_sb_buffer_line(scrollback, get_sb_topline(scrollback) - 1 % get_sb_capacity(scrollback)), shift - i - 1, 0);
            move_sb_up(scrollback, 1);
        }
        wrefresh(window);
    }
}

STATIC void scroll_page_down(BaseWindow *baseWindow) {
    
    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != SCROLLBACK_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    Scrollback *scrollback = get_scrollback(((ScrollbackWindow*)baseWindow));

    int rows = get_window_rows(baseWindow);
    int bottomLines = count_remaining_bottom_lines(scrollback);

    if (bottomLines > 0) {

        int shift = bottomLines >= rows ? rows : bottomLines;
        wscrl(window, shift);

        for (int i = 0; i < shift; i++) {

            print_cx_string(window, get_sb_buffer_line(scrollback, get_sb_bottomline(scrollback) % get_sb_capacity(scrollback)), rows - shift + i, 0);
            move_sb_down(scrollback, 1);
        }
        wrefresh(window);
    }
}

STATIC void move_cursor_left(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    if (move_le_cursor_left(lnEditor)) {
        wmove(window, 0, get_le_cursor(lnEditor));
    }

}

STATIC void move_cursor_right(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    if (move_le_cursor_right(lnEditor)) {
        wmove(window, 0, get_le_cursor(lnEditor));
    }
}

STATIC void use_backspace(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    if (use_le_backspace(lnEditor))  {

        wmove(window, 0, get_le_cursor(lnEditor));
        wdelch(window);
    }
}

STATIC void use_delete(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    if (use_le_delete(lnEditor))  {

        wdelch(window);
    }
}

STATIC void use_home(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    if (use_le_home(lnEditor))  {
        wmove(window, 0, PROMPT_SIZE);
    }
}

STATIC void use_end(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    if (use_le_end(lnEditor))  {
        wmove(window, 0, get_le_char_count(lnEditor) + PROMPT_SIZE);
    }
}

void display_current_command(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));

    Message *message = get_current_item(get_le_front_cmd_queue(lnEditor));

    if (message != NULL) {
        display_command_text(window, lnEditor, get_message_content(message));
    }
}

STATIC void display_previous_command(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));
    Message *message = get_previous_item(get_le_front_cmd_queue(lnEditor));

    if (message != NULL) {
        display_command_text(window, lnEditor, get_message_content(message));
    }
}

STATIC void display_next_command(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != INPUT_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    LineEditor *lnEditor = get_line_editor(((InputWindow*)baseWindow));
    Message *message = get_next_item(get_le_front_cmd_queue(lnEditor));

    if (message != NULL) {
        display_command_text(window, lnEditor, get_message_content(message));
    }
}

STATIC void display_command_text(WINDOW *window, LineEditor *lnEditor, const char *content) {

    if (window == NULL || lnEditor == NULL || content == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    delete_part_line(window, PROMPT_SIZE);
    print_string(window, content, 0, PROMPT_SIZE);

    set_le_char_count(lnEditor, strlen(content));
    set_le_cursor(lnEditor, PROMPT_SIZE + strlen(content));

    wrefresh(window);
}

int remap_ctrl_key(int ch) {

    const char *keystr = keyname(ch);

    /* kUP5 and kDN5 are ncurses' identifiers for 
        CTRL + up arrow and CTRL + down arrow */
    if (keystr != NULL) {
        if (strcmp(keystr, "kUP5") == 0) {
            ch = KEY_CTRLUP;
        }
        else if (strcmp(keystr, "kDN5") == 0) {
            ch = KEY_CTRLDOWN;
        }
    }
    return ch;
}