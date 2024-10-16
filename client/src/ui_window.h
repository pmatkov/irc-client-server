#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "scrollback.h"
#include "line_editor.h"

#include <ncursesw/curses.h>

typedef enum {
    SCROLLBACK,
    LINE_EDITOR,
    NO_BACKING
} BackingType;

typedef union BackingStore BackingStore;
typedef struct UIWindow UIWindow;

UIWindow * create_ui_window(WINDOW *window, void *backingItem, BackingType backingType);
void delete_ui_window(UIWindow *uiWindow);

WINDOW * get_window(UIWindow *uiWindow);
Scrollback * get_scrollback(UIWindow *uiWindow);
LineEditor * get_line_editor(UIWindow *uiWindow);

#endif