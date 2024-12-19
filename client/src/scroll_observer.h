#ifndef SCROLL_OBSERVER_H
#define SCROLL_OBSERVER_H

#include "scroll_subject.h"
#include "i_scroll_observer.h"
#include "base_window.h"
#include "input_window.h"

/**
 * @struct ScrollObserver
 * @brief A structure to observe scroll events.
 *
 */
typedef struct ScrollObserver ScrollObserver;

/**
 * @brief Creates a new ScrollObserver.
 *
 * This function allocates and initializes a new ScrollObserver structure.
 *
 * @param subject Pointer to the ScrollSubject to be observed.
 * @param func Function to be called when a scroll event occurs.
 * @param statusWindow Pointer to the status window.
 * @param inputWindow Pointer to the input window.
 * @return Pointer to the newly created ScrollObserver.
 */
ScrollObserver * create_scroll_observer(ScrollSubject *subject, ScrollNotifyFunc func, BaseWindow *statusWindow, InputWindow *inputWindow);

/**
 * @brief Deletes a ScrollObserver.
 *
 * This function deallocates the memory associated with a ScrollObserver.
 *
 * @param subject Pointer to the ScrollSubject being observed.
 * @param scrollObserver Pointer to the ScrollObserver to be deleted.
 */
void delete_scroll_observer(ScrollSubject *subject, ScrollObserver *scrollObserver);

/**
 * @brief Gets the status window from a ScrollObserver.
 *
 * This function returns the pointer to the status window from the given
 * ScrollObserver.
 *
 * @param scrollObserver Pointer to the ScrollObserver.
 * @return Pointer to the status window.
 */
BaseWindow * get_ob_status_window(ScrollObserver *scrollObserver);

/**
 * @brief Gets the input window from a ScrollObserver.
 *
 * This function returns the pointer to the input window from the given
 * ScrollObserver.
 *
 * @param scrollObserver Pointer to the ScrollObserver.
 * @return Pointer to the input window.
 */
InputWindow * get_ob_input_window(ScrollObserver *scrollObserver);

#endif