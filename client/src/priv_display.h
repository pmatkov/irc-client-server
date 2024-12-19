/* --INTERNAL HEADER--
    used for testing */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "priv_scroll_observer.h"
#include "priv_base_window.h"
#include "priv_input_window.h"
#include "../../libs/src/priv_command.h"

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

typedef struct {
    WINDOW *stdscr;
    BaseWindow *titlewin;
    WindowGroup *mainWindows;
    BaseWindow *statuswin;
    InputWindow *inputwin;
    ScrollObserver *observer;
} WindowManager;

WindowManager * create_window_manager(int sbMultiplier, int cmdHistoryCount);
void delete_window_manager(WindowManager *);

void set_windows_options(WindowManager *windowManager);
void init_colors(int useColor);
void init_ui(WindowManager *windowManager, int useColor);

void display_commands(WindowManager *windowManager, const CommandInfo **commandInfos, int count);
void display_usage(WindowManager *windowManager, const CommandInfo *commandInfo);
void display_response(WindowManager *windowManager, const char *response, ...);
void display_server_message(const char *string, void *arg);
void display_settings(WindowManager *windowManager);
void display_time_status(WindowManager *windowManager);
void display_status(BaseWindow *statusWindow, InputWindow *inputWindow, StatusParams *statusParams, ...);

void resize_ui(WindowManager *windowManager, int useColors);

BaseWindow * get_title_window(WindowManager *windowManager);
WindowGroup * get_main_windows(WindowManager *windowManager);
BaseWindow * get_status_window(WindowManager *windowManager);
InputWindow * get_input_window(WindowManager *windowManager);

#ifdef TEST

void create_window_borders(WindowManager *windowManager, int useColor);
void draw_border(WINDOW *window, int y, int x, int width);
void set_status_params(WINDOW *window, StatusParams *statusParams);
void update_status_message(void *instance, const char *message);
void display_list_item(const char *string, void *arg);

#endif

#endif