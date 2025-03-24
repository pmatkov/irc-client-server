#ifndef I_SCROLL_OBSERVER_H
#define I_SCROLL_OBSERVER_H

/* a function pointer type for scroll notification callbacks */
typedef void (*ScrollNotifyFunc)(void *self, const char *message);

/* interface for scroll observers */
typedef struct {
    void *self;
    ScrollNotifyFunc scrollNotifyFunc; 
} IScrollObserver;

#endif