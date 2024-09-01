#ifndef DISPLAY_H
#define DISPLAY_H

#include "settings.h"
#include "scrollback.h"
#include "command_processor.h"

#include <ncursesw/curses.h>

#define KEY_NEWLINE '\n'

// Unicode block character
#define BLOCK_CHAR L"\u2588"     

#define PROMPT "> "
#define PROMPT_SIZE 2
#define SPACE "    "

#define WHITE 0x00
#define RED 0x01
#define BLUE 0x02
#define MAGENTA 0x03
#define CYAN 0x04
#define CYAN_REV 0x5

// font color of separator, origin and message
#define COLOR_SEP(color) ((color) << (0))
#define COLOR_ORG(color) ((color) << (4))
#define COLOR_MSG(color) ((color) << (8))

// #define COLOR_RESOLVE(color, shift) ((color) >> (shift))    // resolve color

#define PRINT_TS 0x01   // print timestamp
#define PRINT_SEP 0x02  // print separator
#define PRINT_ORG 0x04  // print origin
#define PRINT_MSG 0x08  // print message
#define PRINT_STD 0x0B  // print timestamp, separator and message
#define PRINT_ALL 0xFF  // print timestamp, separator, origin and message

#define get_wwidth(win) getmaxx(win)
#define get_wheight(win) getmaxy(win)

#define delete_line(win) (wmove((win), 0, 0) == ERR ? ERR : wclrtoeol((win)))
#define delete_part_line(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

typedef struct WindowManager WindowManager;
typedef struct MessageParams MessageParams;

WindowManager * create_windows(void);
void delete_windows(WindowManager *);

WINDOW *get_chatwin(WindowManager *windowManager);
WINDOW *get_inputwin(WindowManager *windowManager);

void set_windows_options(WindowManager *windowManager);
void init_colors(Settings *settings);
void create_layout(WindowManager *windowManager, Scrollback *scrollback, Settings *settings);
void draw_window_borders(WindowManager *windowManager);

void printmsg(Scrollback *scrollback, MessageParams msgParams, uint32_t attr);
int string_to_complex_string(const char *string, cchar_t *buffer, int len, uint32_t attr);

void display_commands(Scrollback *scrollback);
void display_usage(Scrollback *scrollback, CommandType commandType, const char *usage);
void display_response(Scrollback *scrollback, const char *info, ...);
void display_status(WindowManager *windowManager, const char *status, ...);
void display_settings(Scrollback *scrollback, Settings *settings);

void handle_resize(WindowManager *windowManager, Scrollback *scrollback);

#endif