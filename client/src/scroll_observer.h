#ifndef SCROLL_OBSERVER_H
#define SCROLL_OBSERVER_H

#include "scroll_subject.h"
#include "i_scroll_observer.h"
#include "base_window.h"
#include "input_window.h"

/* a structure to observe scroll events */
typedef struct ScrollObserver ScrollObserver;

ScrollObserver * create_scroll_observer(ScrollSubject *subject, ScrollNotifyFunc func, BaseWindow *statusWindow, InputWindow *inputWindow);
void delete_scroll_observer(ScrollSubject *subject, ScrollObserver *scrollObserver);

/* gets the status window from a ScrollObserver */
BaseWindow * get_ob_status_window(ScrollObserver *scrollObserver);

/* gets the input window from a ScrollObserver */
InputWindow * get_ob_input_window(ScrollObserver *scrollObserver);

#endif