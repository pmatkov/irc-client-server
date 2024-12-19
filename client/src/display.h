#ifndef DISPLAY_H
#define DISPLAY_H

#include "base_window.h"
#include "input_window.h"
#include "../../libs/src/command.h"

#define MARKER_COUNT 2

typedef enum {
    MAIN_STATUS,
    SIDE_STATUS,
    TIME_STATUS,
    STATUS_TYPE_COUNT
} StatusType;

typedef struct {
    StatusType statusType;
    int textStart;
    int fieldStart;
    int fieldWidth;
    const char *marker[MARKER_COUNT];
    const char *message;
    uint32_t format;
} StatusParams;

/**
 * @struct WindowManager
 * @brief Container for UI windows.
 */
typedef struct WindowManager WindowManager;

/**
 * @brief Create a window manager.
 * 
 * @param sbMultiplier Scrollback multiplier.
 * @param cmdHistoryCount Command history count.
 * @return Pointer to the created WindowManager.
 */
WindowManager * create_window_manager(int sbMultiplier, int cmdHistoryCount);

/**
 * @brief Delete a window manager.
 * 
 * @param windowManager Pointer to the WindowManager to be deleted.
 */
void delete_window_manager(WindowManager *windowManager);

/**
 * @brief Set ncurses modes and options.
 * 
 * @param windowManager Pointer to the WindowManager.
 */
void set_windows_options(WindowManager *windowManager);

/**
 * @brief Initialize ncurses colors.
 * 
 * @param useColor Flag to indicate whether to use colors.
 */
void init_colors(int useColor);

/**
 * @brief Set default layout, including the title, borders, and messages.
 * 
 * @param windowManager Pointer to the WindowManager.
 * @param useColor Flag to indicate whether to use colors.
 */
void init_ui(WindowManager *windowManager, int useColor);

/**
 * @brief Display a list of available commands.
 * 
 * @param windowManager Pointer to the WindowManager.
 * @param commandInfos Array of command information.
 * @param count Number of commands.
 */
void display_commands(WindowManager *windowManager, const CommandInfo **commandInfos, int count);

/**
 * @brief Display usage instructions for a command.
 * 
 * @param windowManager Pointer to the WindowManager.
 * @param commandInfo Pointer to the command information.
 */
void display_usage(WindowManager *windowManager, const CommandInfo *commandInfo);

/**
 * @brief Display app response to user commands.
 * 
 * @param windowManager Pointer to the WindowManager.
 * @param response Response message format string.
 * @param ... Additional arguments for the response format string.
 */
void display_response(WindowManager *windowManager, const char *response, ...);

/**
 * @brief Display message received from the server.
 * 
 * @param string Message string.
 * @param arg Additional argument.
 */
void display_server_message(const char *string, void *arg);

/**
 * @brief Display settings values.
 * 
 * @param windowManager Pointer to the WindowManager.
 */
void display_settings(WindowManager *windowManager);

/**
 * @brief Display time status information.
 * 
 * @param windowManager Pointer to the WindowManager.
 */
void display_time_status(WindowManager *windowManager);

/**
 * @brief Displays the status on the given status window.
 *
 * This function updates the status window with the provided status parameters.
 * Additional arguments can be passed to customize the display further.
 *
 * @param statusWindow Pointer to the BaseWindow where the status will be displayed.
 * @param inputWindow Pointer to the InputWindow for user input.
 * @param statusParams Pointer to the StatusParams containing the status information.
 * @param ... Additional arguments for extended functionality.
 */
void display_status(BaseWindow *statusWindow, InputWindow *inputWindow, StatusParams *statusParams, ...);

/**
 * @brief Resizes the user interface.
 *
 * This function adjusts the size of the user interface elements based on the
 * current window size and optionally applies color settings.
 *
 * @param windowManager A pointer to the WindowManager.
 * @param useColors An integer flag indicating whether to use colors (non-zero)
 *                  or not (zero).
 */
void resize_ui(WindowManager *windowManager, int useColors);

/**
 * @brief Retrieves the title window from the window manager.
 *
 * @param windowManager A pointer to the WindowManager.
 * @return A pointer to the BaseWindow representing the title window.
 * 
 */
BaseWindow * get_title_window(WindowManager *windowManager);

/**
 * @brief Retrieves the main windows group from the window manager.
 *
 * @param windowManager A pointer to the WindowManager.
 * @return A pointer to the WindowGroup representing the main windows.
 */
WindowGroup * get_main_windows(WindowManager *windowManager);

/**
 * @brief Retrieves the status window from the window manager.
 *
 * @param windowManager A pointer to the WindowManager.
 * @return A pointer to the BaseWindow representing the status window.
 */
BaseWindow * get_status_window(WindowManager *windowManager);

/**
 * @brief Retrieves the input window from the window manager.
 *
 * @param windowManager A pointer to the WindowManager.
 * @return A pointer to the InputWindow representing the input window.
 */
InputWindow * get_input_window(WindowManager *windowManager);

#endif