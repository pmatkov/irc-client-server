#include "scrollback.h"
#include "display.h"
#include "main.h"

#include <stdlib.h>

#define MAX_CHARS 256

/* scrollback is a backing buffer for on-screen information,
 here implemented as fixed size circular buffer */
Scrollback * create_scrollback(WINDOW *window, int size) {

    Scrollback *scrollback = (Scrollback*) malloc(sizeof(Scrollback));
    if (scrollback == NULL) {
        failed("Error allocating memory.");
    }

    scrollback->buffer = (cchar_t **) malloc(size * sizeof(cchar_t*));
    if (scrollback->buffer == NULL) {
        failed("Error allocating memory.");
    }

    for (int i = 0; i < size; i++) {

        scrollback->buffer[i] = (cchar_t *) calloc(MAX_CHARS + 1, sizeof(cchar_t));
        if (scrollback->buffer[i] == NULL) {
            failed("Error allocating memory.");
        }
    }

    scrollback->window = window;
    scrollback->head = 0;
    scrollback->tail = 0;
    scrollback->currentLine = 0;
    scrollback->allocatedSize = size;
    scrollback->usedSize = 0; 

    return scrollback;
}

void delete_scrollback(Scrollback *scrollback) {

    if (scrollback != NULL) {
        for (int i = 0; i < scrollback->allocatedSize; i++) {

            free(scrollback->buffer[i]); 
        }
        free(scrollback->buffer);
        free(scrollback);
    }
}

// check if scrollback is empty
int is_empty(Scrollback *scrollback) {
    return scrollback->usedSize == 0;
}

// check if scrollback is full
int is_full(Scrollback *scrollback) {
    return scrollback->usedSize == scrollback->allocatedSize;
}

// get number of lines from the start of the buffer until current line
int get_preceding_ln_count(Scrollback *scrollback) {

    if (scrollback->currentLine >= scrollback->tail) {
        return scrollback->currentLine - scrollback->tail;
    }
    else {
        return (scrollback->usedSize - scrollback->tail) + scrollback->currentLine + 1;
    }
}

// save line to scrollback buffer
void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length) {

    if (is_full(scrollback)) {
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

    int y, x;
    save_cursor(scrollback->window, y, x);

    if (lineWnd != -1) {

        wmove(scrollback->window, lineWnd, 0);
        wadd_wchstr(scrollback->window, scrollback->buffer[lineSb-1]);
        restore_cursor(scrollback->window, y, x);
    }
    else { 
        if (scrollback->usedSize > 1) {
            waddch(scrollback->window, '\n');
        }

        int i = 0;

        while (scrollback->buffer[lineSb-1] && scrollback->buffer[lineSb-1][i].chars[0]) {

            wadd_wch(scrollback->window, &scrollback->buffer[lineSb-1][i++]);
        }
    }
}

// restore lines from scrollback buffer on window resize
void restore_from_scrollback(Scrollback *scrollback) {

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