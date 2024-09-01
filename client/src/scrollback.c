#ifdef TEST
#include "test_scrollback.h"
#else
#include "scrollback.h"
#endif

#include "display.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>

#define MAX_CHARS 512
#define MAX_CHARS 512
#define DEFAULT_SB_MULTIPLIER 5

#ifndef TEST

struct Scrollback {
    WINDOW *window;
    cchar_t **buffer;
    int tail;
    int head;
    int currentLine;
    int allocatedSize;
    int usedSize;
};

#endif

// scrollback stores off screen data
Scrollback * create_scrollback(WINDOW *window, int sbMultiplier) {

    int windowHeight = 1;

    if (window != NULL) {
        windowHeight = get_wheight(window);
    }

    if (sbMultiplier <= 0) {
        sbMultiplier = DEFAULT_SB_MULTIPLIER;
    }

    int sbSize = windowHeight * sbMultiplier;

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
    scrollback->currentLine = 0;
    scrollback->allocatedSize = sbSize;
    scrollback->usedSize = 0; 

    return scrollback;
}

void delete_scrollback(Scrollback *scrollback) {

    if (scrollback != NULL) {
        for (int i = 0; i < scrollback->allocatedSize; i++) {

            free(scrollback->buffer[i]); 
        }
        free(scrollback->buffer);
    }
    free(scrollback);
}

WINDOW * sb_get_window(Scrollback *scrollback) {
    return scrollback->window;
}

int sb_get_head(Scrollback *scrollback) {
    return scrollback->head;
}

int sb_is_empty(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return scrollback->usedSize == 0;
}

int sb_is_full(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return scrollback->usedSize == scrollback->allocatedSize;
}

// get line count from start until current
int get_preceding_ln_count(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (scrollback->currentLine >= scrollback->tail) {
        return scrollback->currentLine - scrollback->tail;
    }
    else {
        return (scrollback->usedSize - scrollback->tail) + scrollback->currentLine + 1;
    }
}

// add line to scrollback buffer
void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length) {

    if (scrollback == NULL || string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (sb_is_full(scrollback)) {
        scrollback->tail = (scrollback->tail + 1) % scrollback->allocatedSize; 
    } else {
        scrollback->usedSize++;
    }

    for (int i = 0; i < length; i++) {
        scrollback->buffer[scrollback->head][i] = string[i];
    }

    scrollback->head = (scrollback->head + 1) % scrollback->allocatedSize;
    scrollback->currentLine = scrollback->head;
}

// print line from scrollback buffer
void print_from_scrollback(Scrollback *scrollback, int lineWnd, int lineSb) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int y, x;
    save_cursor(scrollback->window, y, x);

    if (lineWnd == -1) {

        if (scrollback->usedSize > 1) {
            waddch(scrollback->window, '\n');
        }

        int i = 0;

        while (scrollback->buffer[lineSb-1] && scrollback->buffer[lineSb-1][i].chars[0]) {

            wadd_wch(scrollback->window, &scrollback->buffer[lineSb-1][i++]);
        }
    }
    else { 
        wmove(scrollback->window, lineWnd, 0);
        wadd_wchstr(scrollback->window, scrollback->buffer[lineSb-1]);
        restore_cursor(scrollback->window, y, x);
    }
}

// restore lines from scrollback buffer on window resize
void restore_from_scrollback(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);
    int cols  = get_wwidth(scrollback->window);

    int linesToPrint = (scrollback->usedSize < rows) ? scrollback->usedSize : rows;
    int startLine = (scrollback->usedSize < rows) ? scrollback->tail : (scrollback->head - rows) % scrollback->allocatedSize;

    scrollback->currentLine = startLine;

    for (int i = 0; i < linesToPrint; i++) {
        
        scrollback->currentLine = (scrollback->currentLine + 1) % scrollback->allocatedSize;
        print_from_scrollback(scrollback, i, scrollback->currentLine);
    }
    wmove(scrollback->window, linesToPrint - 1, cols - 1);
}

void scroll_line_up(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);

    if (scrollback->usedSize > rows && get_preceding_ln_count(scrollback) - rows) {

        wscrl(scrollback->window, -1);
        print_from_scrollback(scrollback, 0, get_preceding_ln_count(scrollback) - rows);
        wrefresh(scrollback->window);
        scrollback->currentLine = (scrollback->currentLine - 1) % scrollback->allocatedSize;
    }
}

void scroll_line_down(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);

    if (scrollback->usedSize > rows && scrollback->usedSize - get_preceding_ln_count(scrollback)) {

        wscrl(scrollback->window, 1);
        print_from_scrollback(scrollback, rows-1, (scrollback->currentLine + 1) % scrollback->allocatedSize);
        wrefresh(scrollback->window);
        scrollback->currentLine = (scrollback->currentLine + 1) % scrollback->allocatedSize;
    }
}

void scroll_page_up(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);

    if (scrollback->usedSize > rows && get_preceding_ln_count(scrollback) - rows) {

        int shift = (get_preceding_ln_count(scrollback) >= rows * 2) ? rows : get_preceding_ln_count(scrollback) - rows;
        wscrl(scrollback->window, -shift);

        for (int i = 0; i < shift; i++) {
            print_from_scrollback(scrollback, shift - i - 1, get_preceding_ln_count(scrollback) - rows);
            scrollback->currentLine = (scrollback->currentLine - 1) % scrollback->allocatedSize;
        }
        wrefresh(scrollback->window);
    }
}

void scroll_page_down(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int rows = get_wheight(scrollback->window);

    if (scrollback->usedSize > rows && scrollback->usedSize - get_preceding_ln_count(scrollback)) {

        int shift = (scrollback->usedSize - get_preceding_ln_count(scrollback)) >= rows ? rows : scrollback->usedSize - get_preceding_ln_count(scrollback);
        wscrl(scrollback->window, shift);
        
        for (int i = 0; i < shift; i++) {
            print_from_scrollback(scrollback, rows - shift + i, (scrollback->currentLine + 1) % scrollback->allocatedSize);
            scrollback->currentLine = (scrollback->currentLine + 1) % scrollback->allocatedSize;
        }
        wrefresh(scrollback->window);
    }
}