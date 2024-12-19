#ifndef SCROLL_SUBJECT_H
#define SCROLL_SUBJECT_H

#include "i_scroll_subject.h"

/**
 * @brief Creates a new ScrollSubject with the specified capacity.
 * 
 * @param capacity The maximum number of observers that can be attached to the subject.
 * @return A pointer to the newly created ScrollSubject.
 */
ScrollSubject * create_scroll_subject(int capacity);

/**
 * @brief Deletes a ScrollSubject and frees its associated resources.
 * 
 * @param subject A pointer to the ScrollSubject to be deleted.
 */
void delete_scroll_subject(ScrollSubject *subject);

/**
 * @brief Attaches an observer to the ScrollSubject.
 * 
 * @param subject A pointer to the ScrollSubject.
 * @param observer A pointer to the observer to be attached.
 * @return 0 on success, or a non-zero error code on failure.
 */
int attach_observer(ScrollSubject *subject, IScrollObserver *observer);

/**
 * @brief Detaches an observer from the ScrollSubject.
 * 
 * @param subject A pointer to the ScrollSubject.
 * @param observer A pointer to the observer to be detached.
 * @return 0 on success, or a non-zero error code on failure.
 */
int detach_observer(ScrollSubject *subject, IScrollObserver *observer);

/**
 * @brief Notifies all attached observers with a message.
 * 
 * @param subject A pointer to the ScrollSubject.
 * @param message The message to be sent to the observers.
 */
void notify_scroll_observers(ScrollSubject *subject, const char *message);

#endif