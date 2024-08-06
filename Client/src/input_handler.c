#include "input_handler.h"
#include "display.h"
#include "main.h"

#include <stdlib.h>

#define MAX_CHARS 256

STATIC void scroll_line_up(Scrollback * scrollback);
STATIC void scroll_line_down(Scrollback *scrollback);
STATIC void scroll_page_up(Scrollback *scrollback);
STATIC void scroll_page_down(Scrollback *scrollback);

STATIC void move_cursor_left(LineEditor *lnEditor);
STATIC void move_cursor_right(LineEditor *lnEditor);

STATIC void delete_char(LineEditor *lnEditor);
STATIC void add_char(LineEditor *lnEditor, char ch);
STATIC void use_backspace(LineEditor *lnEditor);
STATIC void use_delete(LineEditor *lnEditor);

// line editor stores user input to enable line editing
LineEditor * create_line_editor(WINDOW *window) {

    LineEditor *lnEditor = (LineEditor *) malloc(sizeof(LineEditor));
    if (lnEditor == NULL) {
        failed("Error allocating memory.");
    }

    lnEditor->buffer = (char*) calloc(MAX_CHARS + 1, sizeof(char));
    
    lnEditor->window = window;
    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 0;

    return lnEditor;
}

void delete_line_editor(LineEditor *lnEditor) {

    if (lnEditor != NULL) {
        free(lnEditor->buffer);
        free(lnEditor);
    }
}

STATIC void move_cursor_left(LineEditor *lnEditor) {

    if (lnEditor->cursor > PROMPT_SIZE) {

        wmove(lnEditor->window, 0, --lnEditor->cursor);
    }
}

STATIC void move_cursor_right(LineEditor *lnEditor) {

    if (lnEditor->cursor - PROMPT_SIZE < lnEditor->charCount) {

        wmove(lnEditor->window, 0, ++lnEditor->cursor);
    }
}

STATIC void scroll_line_up(Scrollback *scrollback) {

    int rows = get_wheight(scrollback->window);

    if (scrollback->usedSize > rows && get_preceding_ln_count(scrollback) - rows) {
        wscrl(scrollback->window, -1);
        print_from_scrollback(scrollback, 0, get_preceding_ln_count(scrollback) - rows);
        wrefresh(scrollback->window);
        scrollback->currentLine = (scrollback->currentLine - 1) % scrollback->allocatedSize;
    }
}

STATIC void scroll_line_down(Scrollback *scrollback) {

    int rows = get_wheight(scrollback->window);

    if (scrollback->usedSize > rows && scrollback->usedSize - get_preceding_ln_count(scrollback)) {

        wscrl(scrollback->window, 1);
        print_from_scrollback(scrollback, rows-1, (scrollback->currentLine + 1) % scrollback->allocatedSize);
        wrefresh(scrollback->window);
        scrollback->currentLine = (scrollback->currentLine + 1) % scrollback->allocatedSize;
    }
}

STATIC void scroll_page_up(Scrollback *scrollback) {

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

STATIC void scroll_page_down(Scrollback *scrollback) {

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

STATIC void delete_char(LineEditor *lnEditor) {

    lnEditor->buffer[lnEditor->cursor-PROMPT_SIZE] = '\0';
    --lnEditor->charCount;
}

STATIC void add_char(LineEditor *lnEditor, char ch) {

    int cols = get_wwidth(lnEditor->window);

    if (lnEditor->cursor < cols && lnEditor->charCount + PROMPT_SIZE < cols) {

        char lastChPos = lnEditor->charCount + PROMPT_SIZE;

        for (int i = lastChPos; i > lnEditor->cursor; i--) {

            char shiftCh = mvwinch(lnEditor->window, 0, i-1);
            mvwaddch(lnEditor->window, 0, i, shiftCh);
            lnEditor->buffer[i-PROMPT_SIZE] = lnEditor->buffer[i-PROMPT_SIZE-1];
        }
        mvwaddch(lnEditor->window, 0, lnEditor->cursor, ch);  

        lnEditor->buffer[lnEditor->cursor - PROMPT_SIZE] = ch;
        lnEditor->charCount++;        
        lnEditor->cursor++;  

    }
}

STATIC void use_backspace(LineEditor *lnEditor) {

    if (lnEditor->cursor > PROMPT_SIZE) {

        --lnEditor->cursor;
        delete_char(lnEditor);

        wmove(lnEditor->window, 0, lnEditor->cursor);
        wdelch(lnEditor->window);
    }
}

STATIC void use_delete(LineEditor *lnEditor) {

    if (lnEditor->cursor - PROMPT_SIZE < lnEditor->charCount) {

        delete_char(lnEditor);
        wdelch(lnEditor->window);
    }
}
