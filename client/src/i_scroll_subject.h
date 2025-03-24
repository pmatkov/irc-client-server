#ifndef I_SCROLL_SUBJECT_H
#define I_SCROLL_SUBJECT_H

#include "i_scroll_observer.h"

/* structure representing a scroll subject that maintains a list of observers */
typedef struct {
    IScrollObserver **observers;
    int count;
    int capacity;
} ScrollSubject;

/* attaches an observer to the scroll subject */
int attach_observer(ScrollSubject *subject, IScrollObserver *observer);

/* detaches an observer from the scroll subject */
int detach_observer(ScrollSubject *subject, IScrollObserver *observer);

#endif