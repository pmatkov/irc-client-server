#ifdef TEST
#include "priv_base_window.h"
#include "../../libs/src/mock.h"
#else
#include "base_window.h"
#endif

#include "../../libs/src/enum_utils.h"
#include "../../libs/src/common.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

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

static const char *WINDOW_TYPE_STRINGS[] = {
    "Base window",
    "Scrollback window",
    "Input window",
    "Unknown"
};

ASSERT_ARRAY_SIZE(WINDOW_TYPE_STRINGS, WINDOW_TYPE_COUNT)

BaseWindow * create_base_window(int rows, int cols, int startY, int startX, WindowType windowType) {

    if (!is_valid_enum_type(windowType, WINDOW_TYPE_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *baseWindow = (BaseWindow *) malloc(sizeof(BaseWindow));
    if (baseWindow == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }
    baseWindow->window = newwin(rows, cols, startY, startX);
    baseWindow->rows = rows;
    baseWindow->cols = cols;
    baseWindow->windowType = windowType;

    return baseWindow;
}

void delete_base_window(BaseWindow *baseWindow) {

    if (baseWindow != NULL) {

        if (baseWindow->window != NULL) {
            delwin(baseWindow->window);
        }
    }
    free(baseWindow);
}

WindowGroup * create_window_group(int capacity) {

    if (capacity <= 0) {
        capacity = DEF_WINDOW_COUNT;
    }

    WindowGroup *windowGroup = (WindowGroup*) malloc(sizeof(WindowGroup));
    if (windowGroup == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    windowGroup->baseWindows = (BaseWindow**) malloc(capacity * sizeof(BaseWindow*));
    if (windowGroup->baseWindows == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < capacity; i++) {
        windowGroup->baseWindows[i] = NULL;
    }

    windowGroup->activeWindowIndex = 0;
    windowGroup->count = 0;
    windowGroup->capacity = capacity;

    return windowGroup;
}

void delete_window_group(WindowGroup *windowGroup) {

    if (windowGroup != NULL) {

        for (int i = 0; i < windowGroup->capacity; i++) {

            free(windowGroup->baseWindows[i]);
        }
        free(windowGroup->baseWindows);
    }

    free(windowGroup);
}

void add_window(WindowGroup *windowGroup, BaseWindow *baseWindow) {

    if (windowGroup == NULL || baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (windowGroup->count < windowGroup->capacity) {
        windowGroup->baseWindows[windowGroup->count] = baseWindow;
        windowGroup->count++;
    }
}

void remove_window(WindowGroup *windowGroup) {

    if (windowGroup == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (windowGroup->count > 0) {

        free(windowGroup->baseWindows[windowGroup->count]);
        windowGroup->count--;
    }
}

BaseWindow * get_active_window(WindowGroup *windowGroup) {

    if (windowGroup == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowGroup->baseWindows[windowGroup->activeWindowIndex];
}

void set_active_window(WindowGroup *windowGroup, int activeWindowIndex) {

    if (windowGroup == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    windowGroup->activeWindowIndex = activeWindowIndex;
}

WINDOW * get_window(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return baseWindow->window;
}

WindowType get_window_type(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return baseWindow->windowType;
}

int get_window_rows(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return baseWindow->rows;
}

int get_window_cols(BaseWindow *baseWindow) {

    if (baseWindow == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return baseWindow->cols;

}