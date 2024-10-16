#ifdef TEST
#include "priv_ui_window.h"
#else
#include "ui_window.h"
#endif

#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

union BackingStore {
    Scrollback *scrollback;
    LineEditor *lnEditor;
};

struct UIWindow {
    WINDOW *window;
    BackingStore *backingStore;
    BackingType backingType;
};

STATIC BackingStore * create_backing_store(void *backingItem, BackingType backingType);
STATIC void delete_backing_store(BackingStore *backingStore, BackingType backingType);

UIWindow * create_ui_window(WINDOW *window, void *backingItem, BackingType backingType) {

    if (backingItem != NULL && backingType == NO_BACKING) {
        FAILED(NULL, ARG_ERROR);
    }

    UIWindow *uiWindow = (UIWindow *) malloc(sizeof(UIWindow));
    if (uiWindow == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }
    uiWindow->window = window;
    uiWindow->backingStore = create_backing_store(backingItem, backingType);
    uiWindow->backingType = backingType;

    return uiWindow;
}

void delete_ui_window(UIWindow *uiWindow) {

    if (uiWindow != NULL) {
        delete_backing_store(uiWindow->backingStore, uiWindow->backingType);

        if (uiWindow->window != NULL) {
            delwin(uiWindow->window);
        }
    }
    free(uiWindow);
}

STATIC BackingStore * create_backing_store(void *backingItem, BackingType backingType) {

    if (backingItem != NULL && backingType == NO_BACKING) {
        FAILED(NULL, ARG_ERROR);
    }

    BackingStore *backingStore = NULL;

    if (backingItem != NULL) {

        backingStore = (BackingStore *) malloc(sizeof(BackingStore));
        if (backingStore == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        }

        if (backingType == SCROLLBACK) {
        backingStore->scrollback = backingItem;
        }
        else if (backingType == LINE_EDITOR) {
            backingStore->lnEditor = backingItem;
        }
    }

    return backingStore;
}

STATIC void delete_backing_store(BackingStore *backingStore, BackingType backingType) {

    if (backingStore != NULL) {
        if (backingType == SCROLLBACK) {
            delete_scrollback(backingStore->scrollback);
        }
        else if (backingType == LINE_EDITOR) {
             delete_line_editor(backingStore->lnEditor);
        }
    }
    free(backingStore);
}

WINDOW * get_window(UIWindow *uiWindow) {

    if (uiWindow == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return uiWindow->window;
}

Scrollback * get_scrollback(UIWindow *uiWindow) {

    if (uiWindow == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    Scrollback *scrollback = NULL;

    if (uiWindow->backingType == SCROLLBACK) {
        scrollback = uiWindow->backingStore->scrollback;
    }
    return scrollback;
}

LineEditor * get_line_editor(UIWindow *uiWindow) {

    if (uiWindow == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    LineEditor *lnEditor = NULL;

    if (uiWindow->backingType == LINE_EDITOR) {
        lnEditor = uiWindow->backingStore->lnEditor;
    }
    return lnEditor;
}