#ifdef TEST
#include "priv_scrollback_window.h"
#include "../../libs/src/mock.h"
#else
#include "scrollback_window.h"
#include "priv_base_window.h"
#endif

#include "../../libs/src/common.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <stdbool.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define DEF_MULTIPLIER 5
#define KEY_CTRLUP 1
#define KEY_CTRLDOWN 2

#ifndef TEST

struct Scrollback {
    cchar_t **buffer;
    int tail;
    int head;
    int topLine;
    int bottomLine;
    int viewportHeight;
    bool isScrolledUp;
    bool isMessagePending;
    ScrollSubject *subject;
    int count;
    int capacity;
};

struct ScrollbackWindow {
    BaseWindow baseWindow;
    Scrollback *scrollback;
};

#endif

ScrollbackWindow * create_scrollback_window(int rows, int cols, int startY, int startX, int multiplier) {

    ScrollbackWindow *scrollbackWindow = (ScrollbackWindow *) malloc(sizeof(ScrollbackWindow));
    if (scrollbackWindow == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    scrollbackWindow->baseWindow.window = newwin(rows, cols, startY, startX);
    scrollbackWindow->baseWindow.rows = rows;
    scrollbackWindow->baseWindow.cols = cols;
    scrollbackWindow->baseWindow.windowType = SCROLLBACK_WINDOW;
    scrollbackWindow->scrollback = create_scrollback(rows, multiplier);

    return scrollbackWindow;
}

void delete_scrollback_window(ScrollbackWindow *scrollbackWindow) {

    if (scrollbackWindow != NULL) {

        delwin(scrollbackWindow->baseWindow.window);

        if (scrollbackWindow->scrollback != NULL) {
            delete_scrollback(scrollbackWindow->scrollback);
        }
    }
    free(scrollbackWindow);
}

Scrollback * create_scrollback(int viewportHeight, int multiplier) {

    if (viewportHeight <= 0) {
        viewportHeight = 1;
    }

    if (multiplier <= 0) {
        multiplier = DEF_MULTIPLIER;
    }

    int capacity = viewportHeight * multiplier;

    Scrollback *scrollback = (Scrollback*) malloc(sizeof(Scrollback));
    if (scrollback == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    scrollback->buffer = (cchar_t **) malloc(capacity * sizeof(cchar_t*));
    if (scrollback->buffer == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < capacity; i++) {

        scrollback->buffer[i] = (cchar_t *) calloc(MAX_CHARS + 1, sizeof(cchar_t));
        if (scrollback->buffer[i] == NULL) {
            FAILED(ALLOC_ERROR, NULL);
        }
    }
    scrollback->head = 0;
    scrollback->tail = 0;
    scrollback->topLine = 0;
    scrollback->bottomLine = 0;
    scrollback->viewportHeight = viewportHeight;
    scrollback->isScrolledUp = 0;
    scrollback->isMessagePending = 0;
    scrollback->subject = create_scroll_subject(1);
    scrollback->capacity = capacity;
    scrollback->count = 0; 

    return scrollback;
}

void delete_scrollback(Scrollback *scrollback) {

    if (scrollback != NULL) {

        for (int i = 0; i < scrollback->capacity; i++) {

            free(scrollback->buffer[i]); 
        }
        delete_scroll_subject(scrollback->subject);
        free(scrollback->buffer);
    }
    free(scrollback);
}

bool is_scrollback_empty(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollback->count == 0;
}

bool is_scrollback_full(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollback->count == scrollback->capacity;
}

void add_to_scrollback(Scrollback *scrollback, cchar_t *string) {

    if (scrollback == NULL || string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int charCount = count_complex_chars(string);

    for (int i = 0; i < charCount; i++) {
        scrollback->buffer[scrollback->head][i] = string[i];
    }

    if (is_scrollback_full(scrollback)) {
        scrollback->tail = (scrollback->tail + 1) % scrollback->capacity; 
    } else {
        scrollback->count++;
    }

    scrollback->head = (scrollback->head + 1) % scrollback->capacity;

    if (!scrollback->isScrolledUp) {

        scrollback->bottomLine = scrollback->head;

        if (scrollback->count > scrollback->viewportHeight) {
            scrollback->topLine = (scrollback->topLine + 1) % scrollback->capacity;
        }
    }
    else {

        if (!scrollback->isMessagePending) {
            notify_scroll_observers(scrollback->subject, "new unread messsage(s)");
            scrollback->isMessagePending = 1;
        }
    }
}

void move_sb_up(Scrollback *scrollback, int count) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int remaining = count_remaining_top_lines(scrollback);
    int shift = remaining >= count ? count : remaining;

    if (shift > 0) {

        if (!scrollback->isScrolledUp) {
            scrollback->isScrolledUp = 1;
        }
        scrollback->topLine = (scrollback->topLine - shift) % scrollback->capacity;
        scrollback->bottomLine = (scrollback->bottomLine - shift) % scrollback->capacity;
        remaining -= shift;
    }
}

void move_sb_down(Scrollback *scrollback, int count) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    int remaining = count_remaining_bottom_lines(scrollback);
    int shift = remaining >= count ? count : remaining;

    if (shift > 0) {
        scrollback->topLine = (scrollback->topLine + shift) % scrollback->capacity;
        scrollback->bottomLine = (scrollback->bottomLine + shift) % scrollback->capacity;
        remaining -= shift;
    }

    if (!remaining &&  scrollback->isScrolledUp) {

        if (scrollback->isMessagePending) {
            notify_scroll_observers(scrollback->subject, "");
            scrollback->isMessagePending = 0;
        }
        scrollback->isScrolledUp = 0;
    }
}

/* count the lines above the topLine (hidden part of 
    the scrollback buffer) */
int count_remaining_top_lines(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = 0;
    int topLine = scrollback->topLine;

    while (topLine != scrollback->tail) {

        topLine = (topLine - 1) % scrollback->capacity;
        count++;
    }
    return count; 
}

/* count the lines below the bottomLine (hidden part of
    the scrollback buffer) */
int count_remaining_bottom_lines(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = 0;
    int bottomLine = scrollback->bottomLine;

    while (bottomLine != scrollback->head) {

        bottomLine = (bottomLine + 1) % scrollback->capacity;
        count++;
    }
    return count; 
}

int get_sb_head(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return scrollback->head;
}

int get_sb_topline(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return scrollback->topLine;
}

void set_sb_topline(Scrollback *scrollback, int topLine) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    scrollback->topLine = topLine;
}

int get_sb_bottomline(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return scrollback->bottomLine;
}

void set_sb_bottomline(Scrollback *scrollback, int bottomLine) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    scrollback->bottomLine = bottomLine;
}

int get_sb_capacity(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return scrollback->capacity;
}

bool is_sb_scrolled_up(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollback->isScrolledUp;
}

void set_sb_is_scrolled_up(Scrollback *scrollback, bool isScrolledUp) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    scrollback->isScrolledUp = isScrolledUp;
}

ScrollSubject * get_scroll_subject(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollback->subject;
}

cchar_t * get_sb_buffer_line(Scrollback *scrollback, int index) {

    if (scrollback == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollback->buffer[index];
}

Scrollback * get_scrollback(ScrollbackWindow *scrollbackWindow) {

    if (scrollbackWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return scrollbackWindow->scrollback;
}

BaseWindow * get_sb_base_window(ScrollbackWindow *scrollbackWindow) {

    if (scrollbackWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return &scrollbackWindow->baseWindow;
}

