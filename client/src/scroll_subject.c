#ifdef TEST
#include "priv_scroll_subject.h"
#else
#include "scroll_subject.h"
#endif

#include "i_scroll_observer.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

ScrollSubject * create_scroll_subject(int capacity) {

    ScrollSubject *subject = (ScrollSubject*) malloc(sizeof(ScrollSubject));
    if (subject == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    subject->observers = (IScrollObserver**) malloc(capacity * sizeof(IScrollObserver*));
    if (subject->observers == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < capacity; i++) {

        subject->observers[i] = NULL;
    }

    subject->count = 0;
    subject->capacity = capacity;

    return subject;
}

void delete_scroll_subject(ScrollSubject *subject) {

    if (subject != NULL) {

        for (int i = 0; i < subject->capacity; i++) {

            if (subject->observers[i] != NULL) {
                free(subject->observers[i]);
            }
        }
        free(subject->observers);
    }
    free(subject);
}

int attach_observer(ScrollSubject *subject, IScrollObserver *iObserver) {

    if (subject == NULL || iObserver == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int attached = 0;
    
    for (int i = 0; i < subject->capacity && !attached; i++) {

        if (subject->observers[i] == NULL) {
            subject->observers[i] = iObserver;
            subject->count++;
            attached = 1;
        }
    }
    return attached;
}

int detach_observer(ScrollSubject *subject, IScrollObserver *iObserver) {

    if (subject == NULL || iObserver == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int detached = 0;

    for (int i = 0; i < subject->capacity && !detached; i++) {

        if (subject->observers[i]->self == iObserver->self) {
            free(subject->observers[i]);
            subject->observers[i] = NULL;
            subject->count--;
            detached = 1;
        }
    }
    return detached;
}

void notify_scroll_observers(ScrollSubject *subject, const char *message) {

    if (subject == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    for (int i = 0, notified = 0; i < subject->capacity && notified < subject->count; i++) {

        if (subject->observers[i] != NULL) {
            subject->observers[i]->scrollNotifyFunc(subject->observers[i]->self, message);
            notified++;
        }
    }
}
