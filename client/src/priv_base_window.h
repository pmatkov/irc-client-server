/* --INTERNAL HEADER--
    used for testing */
#ifndef PRIV_BASE_WINDOW_H
#define PRIV_BASE_WINDOW_H

#include "base_window.h"

#define DEF_WINDOW_COUNT 5

struct BaseWindow {
    WINDOW *window;
    int rows;
    int cols;
    WindowType windowType;
};

struct WindowGroup {
    BaseWindow **baseWindows;
    int activeWindowIndex;
    int count;
    int capacity;
};

#endif