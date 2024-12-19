#ifdef TEST
#include "priv_scroll_observer.h"
#else
#include "scroll_observer.h"
#endif

#include "i_scroll_subject.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct ScrollObserver {
    BaseWindow *statusWindow;
    InputWindow *inputWindow;
};

#endif

ScrollObserver * create_scroll_observer(ScrollSubject *subject, ScrollNotifyFunc func, BaseWindow *statusWindow, InputWindow *inputWindow) {

    if (subject == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    ScrollObserver *scrollObserver = (ScrollObserver*) malloc(sizeof(ScrollObserver));
    if (scrollObserver == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    scrollObserver->statusWindow = statusWindow;
    scrollObserver->inputWindow = inputWindow;

    IScrollObserver *iObserver = (IScrollObserver*) malloc(sizeof(IScrollObserver));
    if (iObserver == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    iObserver->self = scrollObserver;
    iObserver->scrollNotifyFunc = func;

    attach_observer(subject, iObserver);

    return scrollObserver;
}

void delete_scroll_observer(ScrollSubject *subject, ScrollObserver *scrollObserver) {
    
    if (subject == NULL || scrollObserver == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    IScrollObserver iObserver = {0};

    iObserver.self = scrollObserver;
    iObserver.scrollNotifyFunc = NULL;

    detach_observer(subject, &iObserver);

    free(scrollObserver);
}

BaseWindow * get_ob_status_window(ScrollObserver *scrollObserver) {

    if (scrollObserver == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollObserver->statusWindow;
}

InputWindow * get_ob_input_window(ScrollObserver *scrollObserver) {

    if (scrollObserver == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return scrollObserver->inputWindow;
}
