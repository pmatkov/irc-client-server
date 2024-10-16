#ifdef TEST
#include "priv_scrollback.h"
#else
#include "scrollback.h"
#endif

#include "display.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>

#define KEY_CTRLUP 1
#define KEY_CTRLDOWN 2

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

/* a ScrollbackCmd adjusts a visible portion 
    of the scrollback buffer. each scrollback 
    command is mapped to a function which 
    performs a desired scrollback action */
struct ScrollbackCmd {
    int keyCode;
    ScrollbackFunc scrollbackFunc;
};

/* a scrollback is a fixed size circular buffer.
    when this buffer is full, the next line to
    be added will overwrite the oldest in the 
    buffer. the head refers to the last line 
    and the tail to the first line in the 
    buffer. the topLine and the bottomLine 
    define the visible part of the scrollback */

struct Scrollback {
    WINDOW *window;
    cchar_t **buffer;
    int tail;
    int head;
    int topLine;
    int bottomLine;
    int capacity;
    int count;
};

#endif

STATIC int count_visible_lines(Scrollback *scrollback);
STATIC int count_remaining_top_lines(Scrollback *scrollback);
STATIC int count_remaining_bottom_lines(Scrollback *scrollback);

static const ScrollbackCmd SCROLLBACK_CMD[] = {
    {KEY_PPAGE, scroll_page_up},
    {KEY_NPAGE, scroll_page_down},
    {KEY_CTRLUP, scroll_line_up},
    {KEY_CTRLDOWN, scroll_line_down},
};

/* the size of the scrollback can be adjusted. 
    a minimum scrollback size is usually the 
    size of one "chat window". the sizeMultiplier
    indicates how much space will be allocated
    to the scrollback relative to the size of
    that window. if the sizeMultiplier is 0, 
    a default value will be used */

Scrollback * create_scrollback(WINDOW *window, int sizeMultiplier) {

    int windowHeight = 1;
    const int DEFAULT_SIZE_MULTIPLIER = 5;

    if (window != NULL) {
        windowHeight = get_wheight(window);
    }

    if (sizeMultiplier <= 0) {
        sizeMultiplier = DEFAULT_SIZE_MULTIPLIER;
    }

    int sbSize = windowHeight * sizeMultiplier;

    Scrollback *scrollback = (Scrollback*) malloc(sizeof(Scrollback));
    if (scrollback == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    scrollback->buffer = (cchar_t **) malloc(sbSize * sizeof(cchar_t*));
    if (scrollback->buffer == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    for (int i = 0; i < sbSize; i++) {

        scrollback->buffer[i] = (cchar_t *) calloc(MAX_CHARS + 1, sizeof(cchar_t));
        if (scrollback->buffer[i] == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        }
    }

    scrollback->window = window;
    scrollback->head = 0;
    scrollback->tail = 0;
    scrollback->topLine = 0;
    scrollback->bottomLine = 0;
    scrollback->capacity = sbSize;
    scrollback->count = 0; 

    return scrollback;
}

void delete_scrollback(Scrollback *scrollback) {

    if (scrollback != NULL) {
        for (int i = 0; i < scrollback->capacity; i++) {

            free(scrollback->buffer[i]); 
        }
        free(scrollback->buffer);
    }
    free(scrollback);
}

int is_scrollback_empty(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return scrollback->count == 0;
}

int is_scrollback_full(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return scrollback->count == scrollback->capacity;
}

void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length) {

    if (scrollback == NULL || string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    /* when the scrollback is full, the tail 
        will move to create space for additional
        lines */

    if (is_scrollback_full(scrollback)) {
        scrollback->tail = (scrollback->tail + 1) % scrollback->capacity; 
    } else {
        scrollback->count++;
    }

    for (int i = 0; i < length; i++) {
        scrollback->buffer[scrollback->head][i] = string[i];
    }

    scrollback->head = (scrollback->head + 1) % scrollback->capacity;

    scrollback->bottomLine = scrollback->head;

    if (scrollback->count > get_wheight(scrollback->window)) {
       scrollback->topLine = (scrollback->topLine + 1) % scrollback->capacity;
    }
}

void print_from_scrollback(Scrollback *scrollback, int lineWnd, int lineSb) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (lineWnd < -1 || lineWnd > get_wheight(scrollback->window)) {
        FAILED("Invalid window line %d", NO_ERRCODE, lineWnd);
    }

    if (lineSb < 1 || lineSb > scrollback->capacity) {
        FAILED("Invalid scrollback line %d", NO_ERRCODE, lineSb);
    }

    int y, x;

    if (lineWnd == -1) {

        /*  a primary print operation leaves cursor at the end
            of the printed text. therefore, before printing
            the next line of text, the cursor must be moved 
            to the next row */

        save_cursor(scrollback->window, y, x);

        if (x > 0) {
            waddch(scrollback->window, '\n');
        }

        int i = 0;

        /* wadd_ch moves cursor with each added char */
        while (scrollback->buffer[lineSb-1] && scrollback->buffer[lineSb-1][i].chars[0]) {

            wadd_wch(scrollback->window, &scrollback->buffer[lineSb-1][i++]);
        }
    }
    else {
        /* a secondary print operation is used for 
            scrolling. when the window is scrolled, a
            blank space that is left behind must be filled 
            with lines from the scrollback buffer. 
            for this purpose, wadd_wchstr function is used.
            this function doesn't move the cursor when printing
            chars */

        save_cursor(scrollback->window, y, x);

        wmove(scrollback->window, lineWnd, 0);
        wadd_wchstr(scrollback->window, scrollback->buffer[lineSb-1]);

        restore_cursor(scrollback->window, y, x);
    }
}

void restore_from_scrollback(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    /* the visible part of the scrollback will be
        repainted on the screen during a resize event,
        as long as least one line of the "chat window"
        remains visible. the bottomLine marker poosition
        will be adjusted accordingly */

    int rows = get_wheight(scrollback->window);

    if (rows) {

        wmove(scrollback->window, 0, 0);

        LOG(INFO, "tl: %d ", scrollback->topLine, scrollback->bottomLine);

        for (int i = scrollback->topLine; i < rows && i < scrollback->head; i++) {

            print_from_scrollback(scrollback, -1, i + 1);
            scrollback->bottomLine = (i + 1) % scrollback->capacity;
        }
        wrefresh(scrollback->window);
    }

    LOG(INFO, "tl: %d bl: %d, rows: %d, capacity: %d, count: %d", scrollback->topLine, scrollback->bottomLine, rows, scrollback->capacity, scrollback->count);
}

void scroll_line_up(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);

    /* the bottomLine position won't be adjusted
        if the window was expanded and there is an 
        additional space which may be used to 
        display more lines from the scrollback
        buffer */

    int topLines = count_remaining_top_lines(scrollback);
    int fixed = count_visible_lines(scrollback) < rows;

    if (topLines) {

        wscrl(scrollback->window, -1);
        print_from_scrollback(scrollback, 0, scrollback->topLine);
        wrefresh(scrollback->window);
        scrollback->topLine = (scrollback->topLine - 1) % scrollback->capacity;

        if (!fixed) {
            scrollback->bottomLine = (scrollback->bottomLine - 1) % scrollback->capacity;
        }
    }
}

void scroll_line_down(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);
    int bottomLines = count_remaining_bottom_lines(scrollback);

    if (bottomLines) {

        LOG(INFO, "bl: %d head: %d bottomLines: %d", scrollback->bottomLine, scrollback->head, bottomLines);

        wscrl(scrollback->window, 1);
        print_from_scrollback(scrollback, rows - 1, scrollback->bottomLine + 1);
        wrefresh(scrollback->window);
        scrollback->topLine = (scrollback->topLine + 1) % scrollback->capacity;
        scrollback->bottomLine = (scrollback->bottomLine + 1) % scrollback->capacity;

    }
}

void scroll_page_up(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);
    int topLines = count_remaining_top_lines(scrollback);

    if (topLines) {

        int shift = topLines >= rows ? rows : topLines;

        wscrl(scrollback->window, -shift);

        for (int i = 0; i < shift; i++) {
            print_from_scrollback(scrollback, shift - i - 1, scrollback->topLine % scrollback->capacity);
            scrollback->topLine = (scrollback->topLine - 1) % scrollback->capacity;
            scrollback->bottomLine = (scrollback->bottomLine - 1) % scrollback->capacity;
        }
        wrefresh(scrollback->window);
    }
}

void scroll_page_down(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);
    int bottomLines = count_remaining_bottom_lines(scrollback);

    if (bottomLines) {

        int shift = bottomLines >= rows ? rows : bottomLines;
        wscrl(scrollback->window, shift);
        
        for (int i = 0; i < shift; i++) {
            print_from_scrollback(scrollback, rows - shift + i, (scrollback->bottomLine + 1) % scrollback->capacity);
            scrollback->topLine = (scrollback->topLine + 1) % scrollback->capacity;
            scrollback->bottomLine = (scrollback->bottomLine + 1) % scrollback->capacity;
        }
        wrefresh(scrollback->window);
    }
}

/* count the lines between the topLIne and the bottomLine
    (the visible part of the scrollback) */
STATIC int count_visible_lines(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int count = 0;

    if (scrollback->bottomLine - scrollback->topLine >= 0) {
        count = scrollback->bottomLine - scrollback->topLine;
    }
    else {
        count = scrollback->bottomLine + scrollback->capacity - scrollback->topLine;
    }

    return count;
}

/* count lines above the topLine (hidden
    part of the scrollback) */
STATIC int count_remaining_top_lines(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int count = 0;
    int topLine = scrollback->topLine;

    while (topLine != scrollback->tail) {

        topLine = (topLine - 1) % scrollback->capacity;
        count++;
    }

    return count; 
}

/* count ines below the bottomLine (hidden
    part of the scrollback) */
STATIC int count_remaining_bottom_lines(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int count = 0;
    int bottomLine = scrollback->bottomLine;

    while (bottomLine != scrollback->head) {

        bottomLine= (bottomLine + 1) % scrollback->capacity;
        count++;
    }

    return count; 
}

ScrollbackFunc get_scrollback_function(int index) {

    ScrollbackFunc scrollbackFunc = NULL;

    if (index >= 0 && index < ARR_SIZE(SCROLLBACK_CMD)) {

        scrollbackFunc = SCROLLBACK_CMD[index].scrollbackFunc;
    }
    return scrollbackFunc;
}

int get_sb_func_index(int keyCode) {

    int cmdIndex = -1;

    for (int i = 0; i < ARR_SIZE(SCROLLBACK_CMD) && cmdIndex == -1; i++) {

        if (SCROLLBACK_CMD[i].keyCode == keyCode) {
            cmdIndex = i;
        }
    }
    return cmdIndex;
}

int remap_ctrl_key(int ch) {

    /* kUP5 and kDN5 are ncurses identifiers
        which represent key combinations CTRL + up and 
        down arrow */
    const char *keystr = keyname(ch);

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

WINDOW * get_scrollback_window(Scrollback *scrollback) {
    return scrollback->window;
}

int get_scrollback_head(Scrollback *scrollback) {
    return scrollback->head;
}