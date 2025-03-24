#ifndef SCROLL_SUBJECT_H
#define SCROLL_SUBJECT_H

#include "i_scroll_subject.h"

ScrollSubject * create_scroll_subject(int capacity);
void delete_scroll_subject(ScrollSubject *subject);

/* attaches an observer to the ScrollSubject */
int attach_observer(ScrollSubject *subject, IScrollObserver *observer);

/*detaches an observer from the ScrollSubject  */
int detach_observer(ScrollSubject *subject, IScrollObserver *observer);

/* notifies all attached observers with a message */
void notify_scroll_observers(ScrollSubject *subject, const char *message);

#endif