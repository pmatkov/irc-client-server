#ifndef BASE_WINDOW_H
#define BASE_WINDOW_H

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

/**
 * @enum WindowType
 * @brief Enum representing different types of windows.
 * 
 */
typedef enum {
    BASE_WINDOW,
    SCROLLBACK_WINDOW,
    INPUT_WINDOW,
    UNKNOWN_WINDOW_TYPE,
    WINDOW_TYPE_COUNT
} WindowType;

/**
 * @struct BaseWindow
 * @brief A base window type.
 */
typedef struct BaseWindow BaseWindow;

/**
 * @struct WindowGroup
 * @brief A group consists of at least one window and possibly more. 
 * Only one window in the group is visible at any one time. 
 * Used for creating additional windows when user joins a channel. 
 * In that case, a channel chat window becomes visible instead of the main window.
 */
typedef struct WindowGroup WindowGroup;

/**
 * @brief Creates a base window.
 * 
 * @param rows Number of rows in the window.
 * @param cols Number of columns in the window.
 * @param startY Starting Y position of the window.
 * @param startX Starting X position of the window.
 * @param windowType Type of the window.
 * @return Pointer to the created BaseWindow.
 */
BaseWindow * create_base_window(int rows, int cols, int startY, int startX, WindowType windowType);

/**
 * @brief Deletes a base window.
 * 
 * @param baseWindow Pointer to the BaseWindow to be deleted.
 */
void delete_base_window(BaseWindow *baseWindow);

/**
 * @brief Creates a window group.
 * 
 * @param capacity Capacity of the window group.
 * @return Pointer to the created WindowGroup.
 */
WindowGroup * create_window_group(int capacity);

/**
 * @brief Deletes a window group.
 * 
 * @param windowGroup Pointer to the WindowGroup to be deleted.
 */
void delete_window_group(WindowGroup *windowGroup);

/**
 * @brief Adds a window to the window group.
 * 
 * @param windowGroup Pointer to the WindowGroup.
 * @param baseWindow Pointer to the BaseWindow to be added.
 */
void add_window(WindowGroup *windowGroup, BaseWindow *baseWindow);

/**
 * @brief Removes the active window from the window group.
 * 
 * @param windowGroup Pointer to the WindowGroup.
 */
void remove_window(WindowGroup *windowGroup);

/**
 * @brief Gets the active window in the window group.
 * 
 * @param windowGroup Pointer to the WindowGroup.
 * @return Pointer to the active BaseWindow.
 */
BaseWindow * get_active_window(WindowGroup *windowGroup);

/**
 * @brief Sets the active window in the window group.
 * 
 * @param windowGroup Pointer to the WindowGroup.
 * @param activeWindowIndex Index of the window to be set as active.
 */
void set_active_window(WindowGroup *windowGroup, int activeWindowIndex);

/**
 * @brief Gets the WINDOW structure of the base window.
 * 
 * @param baseWindow Pointer to the BaseWindow.
 * @return Pointer to the WINDOW structure.
 */
WINDOW * get_window(BaseWindow *baseWindow);

/**
 * @brief Gets the type of the base window.
 * 
 * @param baseWindow Pointer to the BaseWindow.
 * @return Type of the window.
 */
WindowType get_window_type(BaseWindow *baseWindow);

/**
 * @brief Gets the number of rows in the base window.
 * 
 * @param baseWindow Pointer to the BaseWindow.
 * @return Number of rows.
 */
int get_window_rows(BaseWindow *baseWindow);

/**
 * @brief Gets the number of columns in the base window.
 * 
 * @param baseWindow Pointer to the BaseWindow.
 * @return Number of columns.
 */
int get_window_cols(BaseWindow *baseWindow);

#endif