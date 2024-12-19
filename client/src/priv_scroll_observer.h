/* --INTERNAL HEADER--
    used for testing */
#ifndef SCROLL_OBSERVER_H
#define SCROLL_OBSERVER_H

#include "priv_base_window.h"
#include "priv_input_window.h"
#include "priv_scroll_subject.h"
#include "i_scroll_observer.h"

typedef struct {
    BaseWindow *statusWindow;
    InputWindow *inputWindow;
} ScrollObserver;

ScrollObserver * create_scroll_observer(ScrollSubject *subject, ScrollNotifyFunc func, BaseWindow *statusWindow, InputWindow *inputWindow);
void delete_scroll_observer(ScrollSubject *subject, ScrollObserver *scrollObserver);

BaseWindow * get_ob_status_window(ScrollObserver *scrollObserver);
InputWindow * get_ob_input_window(ScrollObserver *scrollObserver);

#endif