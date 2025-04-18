/* --INTERNAL HEADER--
    used for testing */
#ifndef SCROLL_SUBJECT_H
#define SCROLL_SUBJECT_H

#include "i_scroll_subject.h"

ScrollSubject * create_scroll_subject(int capacity);
void delete_scroll_subject(ScrollSubject *subject);

int attach_observer(ScrollSubject *subject, IScrollObserver *observer);
int detach_observer(ScrollSubject *subject, IScrollObserver *observer);

void notify_scroll_observers(ScrollSubject *subject, const char *message);

#endif