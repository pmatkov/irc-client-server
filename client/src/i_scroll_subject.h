#ifndef I_SCROLL_SUBJECT_H
#define I_SCROLL_SUBJECT_H

#include "i_scroll_observer.h"

/**
 * @brief Structure representing a scroll subject that maintains a list of observers.
 */
typedef struct {
    IScrollObserver **observers;
    int count;
    int capacity;
} ScrollSubject;

/**
 * @brief Attaches an observer to the scroll subject.
 *
 * @param subject Pointer to the ScrollSubject.
 * @param observer Pointer to the IScrollObserver to be attached.
 * @return int 0 on success, non-zero on failure.
 */
int attach_observer(ScrollSubject *subject, IScrollObserver *observer);

/**
 * @brief Detaches an observer from the scroll subject.
 *
 * @param subject Pointer to the ScrollSubject.
 * @param observer Pointer to the IScrollObserver to be detached.
 * @return int 0 on success, non-zero on failure.
 */
int detach_observer(ScrollSubject *subject, IScrollObserver *observer);

#endif