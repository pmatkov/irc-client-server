/* --INTERNAL HEADER--
    used for testing */
#ifndef DISPLAY_H
#define DISPLAY_H

#define NCURSES_WIDECHAR 1

#include "priv_scrollback.h"
#include "priv_line_editor.h"
#include "priv_ui_window.h"
#include "../../libs/src/priv_command.h"

#include <ncursesw/curses.h>

#define KEY_NEWLINE '\n'

#define BLOCK_CHAR L"\u2588"     

#define PROMPT "> "
#define PROMPT_SIZE 2
#define SPACE "    "
#define SPACE_2 "        "

#define BITS_PER_HEX 4

#define COMPRESS_BITS(hex1, hex2, value) ((value) >> (((hex2) - 1) * BITS_PER_HEX) ) | (((value) >> ((hex1) * BITS_PER_HEX)) & 0xF)

#define get_remaining_cchars(array) ARR_SIZE(array) - count_complex_chars(array)

#define BIT_MASK_SEP 0x00F00F
#define BIT_MASK_ORG 0x0F00F0
#define BIT_MASK_CNT 0xF00F00

#define WHITE 0x00
#define RED 0x01
#define BLUE 0x02
#define MAGENTA 0x03
#define CYAN 0x04
#define CYAN_REV 0x5

#define COLOR_SEP(color) ((color) << (0))
#define COLOR_ORG(color) ((color) << (4))
#define COLOR_CNT(color) ((color) << (8))

#define STYLE_SEP(style) ((style) << (12))
#define STYLE_ORG(style) ((style) << (16))
#define STYLE_CNT(style) ((style) << (20))

#define get_wwidth(win) getmaxx(win)
#define get_wheight(win) getmaxy(win)

#define delete_line(win) (wmove((win), 0, 0) == ERR ? ERR : wclrtoeol((win)))
#define delete_part_line(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

typedef enum {
    NORMAL, 
    BOLD, 
    STANDOUT, 
    DIM, 
    ITALIC,
    ATTR_COUNT
} Attributes;

typedef struct {
    WINDOW *stdscr;
    UIWindow *titlewin;
    UIWindow *chatwin;
    UIWindow *statuswin;
    UIWindow *inputwin;
} WindowManager;

typedef struct {
    int useTimestamp;
    const char *separator;
    const char *origin;
    const char *content;
    uint32_t format;
} PrintTokens;

typedef void(*PrintFunc)(WindowManager *windowManager, cchar_t *string, int size);

WindowManager * create_windows(int sbMultiplier);
void delete_windows(WindowManager *);

PrintTokens * create_print_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format);
void delete_print_tokens(PrintTokens *printTokens);

void set_windows_options(WindowManager *windowManager);
void init_colors(int useColor);
void init_ui(WindowManager *windowManager, int useColor);

void printstr(PrintTokens *printTokens, WindowManager *windowManager);
int string_to_complex_string(cchar_t *buffer, int size, const char *string, uint32_t format);
int count_complex_chars(cchar_t *string);

void display_commands(WindowManager *windowManager, const CommandInfo *commands, int count);
void display_usage(WindowManager *windowManager, const CommandInfo *command);
void display_response(WindowManager *windowManager, const char *response, ...);
void display_settings(WindowManager *windowManager);
void display_time(WindowManager *windowManager);
void display_status(WindowManager *windowManager, const char *status, ...);
void display_server_message(const char *string, void *arg);

void resize_ui(WindowManager *windowManager, int useColors);

UIWindow * get_titlewin(WindowManager *windowManager);
UIWindow * get_chatwin(WindowManager *windowManager);
UIWindow * get_statuswin(WindowManager *windowManager);
UIWindow * get_inputwin(WindowManager *windowManager);

PrintFunc get_print_function(void);
void set_print_function(PrintFunc pf);

#ifdef TEST

void create_window_borders(WindowManager *windowManager, int useColor);
void draw_border(WINDOW *window, int y, int x, int width);
void print_complex_string(WindowManager *windowManager, cchar_t *string, int size);
void display_list_item(const char *string, void *arg);
// void display_string_list(WindowManager *windowManager, const char **stringList, int count, const char *title);

#endif

#endif