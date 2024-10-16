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

// Unicode block character
#define BLOCK_CHAR L"\u2588"     

#define PROMPT "> "
#define PROMPT_SIZE 2
#define SPACE "    "
#define SPACE_2 "        "

#define BITS_PER_HEX 4

/* compress bits from 0x<abc><def> where
    bit x_i, i ∈ {a, b, c} and bit y_i, 
    i ∈ {d, e, f} are combined to form 
    0x<x]><y> */
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

// font color of separator, origin and message
#define COLOR_SEP(color) ((color) << (0))
#define COLOR_ORG(color) ((color) << (4))
#define COLOR_CNT(color) ((color) << (8))

#define ATTR_SEP(attr) ((attr) << (12))
#define ATTR_ORG(attr) ((attr) << (16))
#define ATTR_CNT(attr) ((attr) << (20))

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

typedef enum {
    NORMAL, 
    BOLD, 
    STANDOUT, 
    DIM, 
    ITALIC,
    ATTR_COUNT
} Attributes;

// typedef struct {
//     WINDOW *stdscr;
//     WINDOW *titlewin;
//     WINDOW *chatwin;
//     WINDOW *statuswin;
//     WINDOW *inputwin;
// } WindowManager;

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

void display_commands(WindowManager *windowManager, const Command *commands, int count);
void display_usage(WindowManager *windowManager, const Command *command);
void display_response(WindowManager *windowManager, const char *response, ...);
void display_settings(WindowManager *windowManager);
void display_status(WindowManager *windowManager, const char *status, ...);

void resize_ui(WindowManager *windowManager, int useColors);

UIWindow * get_titlewin(WindowManager *windowManager);
UIWindow * get_chatwin(WindowManager *windowManager);
UIWindow * get_statuswin(WindowManager *windowManager);
UIWindow * get_inputwin(WindowManager *windowManager);

PrintFunc get_print_function(void);
void set_print_function(PrintFunc pf);

#ifdef TEST

void create_window_borders(WindowManager *windowManager, int useColor);
void print_complex_string(WindowManager *windowManager, cchar_t *string, int size);
void display_string_list(WindowManager *windowManager, const char **stringList, int count, const char *title);

#endif

#endif