#ifndef I_SCROLL_OBSERVER_H
#define I_SCROLL_OBSERVER_H

/**
 * @typedef ScrollNotifyFunc
 * @brief A function pointer type for scroll notification callbacks.
 * 
 * @param self A pointer to the object instance.
 * @param message A message associated with the scroll event.
 */
typedef void (*ScrollNotifyFunc)(void *self, const char *message);

/**
 * @struct IScrollObserver
 * @brief Interface for scroll observers.
 * 
 */
typedef struct {
    void *self;
    ScrollNotifyFunc scrollNotifyFunc; 
} IScrollObserver;

#endif