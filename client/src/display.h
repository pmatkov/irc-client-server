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

/* container for UI windows */
typedef struct WindowManager WindowManager;

WindowManager * create_window_manager(int sbMultiplier, int cmdHistoryCount);
void delete_window_manager(WindowManager *windowManager);

/* set ncurses modes and options */
void set_windows_options(WindowManager *windowManager);

/* initialize ncurses colors */
void init_colors(int useColor);

/* set default layout, including the title, borders, and messages */
void init_ui(WindowManager *windowManager, int useColor);

/* display a list of available commands */
void display_commands(WindowManager *windowManager, const CommandInfo **commandInfos, int count);

/* display usage instructions for a command */
void display_usage(WindowManager *windowManager, const CommandInfo *commandInfo);

/* display app response to user commands */
void display_response(WindowManager *windowManager, const char *response, ...);

void display_server_message(const char *string, void *arg);

void display_settings(WindowManager *windowManager);
void display_time_status(WindowManager *windowManager);
void display_status(BaseWindow *statusWindow, InputWindow *inputWindow, StatusParams *statusParams, ...);

void resize_ui(WindowManager *windowManager, int useColors);

/* retrieves windows from the window manager */
BaseWindow * get_title_window(WindowManager *windowManager);
WindowGroup * get_main_windows(WindowManager *windowManager);
BaseWindow * get_status_window(WindowManager *windowManager);
InputWindow * get_input_window(WindowManager *windowManager);

#endif