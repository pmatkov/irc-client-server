#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "scrollback.h"
#include "line_editor.h"

#include <ncursesw/curses.h>

/* represents different types of backing
    stores for the ncurses window */
typedef enum {
    SCROLLBACK,
    LINE_EDITOR,
    NO_BACKING
} BackingType;

/* a backing store is used to save both 
on-screen and off-screen data, providing 
control over the display of that data */
typedef union BackingStore BackingStore;

/* represents container for ncurses window 
    and an optional backing store */
typedef struct UIWindow UIWindow;

UIWindow * create_ui_window(WINDOW *window, void *backingItem, BackingType backingType);
void delete_ui_window(UIWindow *uiWindow);

WINDOW * get_window(UIWindow *uiWindow);
Scrollback * get_scrollback(UIWindow *uiWindow);
LineEditor * get_line_editor(UIWindow *uiWindow);

#endif