#ifndef DISPLAY_H
#define DISPLAY_H

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include "ui_window.h"
#include "scrollback.h"
#include "line_editor.h"
#include "../../libs/src/command.h"

#include <ncursesw/curses.h>

#define KEY_NEWLINE '\n'

/* used to create window borders */
#define BLOCK_CHAR L"\u2588"

/* prompt for the "input window" */
#define PROMPT "> "
#define PROMPT_SIZE 2

#define SPACE "    "
#define SPACE_2 "        "

#define BITS_PER_HEX 4

/* let 0xabcdef be a number containing color and
    style attributes of the text. [abc] and [def]
    represent groups of three hex digits each, 
    or 24 bits in total. out of these 24 bits, 
    4 bits in each group are active. this macro
    shifts position of those 4 + 4 bits so that
    they occupy positons e and f in the initial 
    number */
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

/* create a bit field for text colors and
    styles to be applied during printing */
#define COLOR_SEP(color) ((color) << (0))
#define COLOR_ORG(color) ((color) << (4))
#define COLOR_CNT(color) ((color) << (8))

#define STYLE_SEP(style) ((style) << (12))
#define STYLE_ORG(style) ((style) << (16))
#define STYLE_CNT(style) ((style) << (20))

#define get_wwidth(win) getmaxx(win)
#define get_wheight(win) getmaxy(win)

/* delete complete or partial line on the
    screen (partial delete deletes the text
    from the cursor until the end of the 
    line) */
#define delete_line(win) (wmove((win), 0, 0) == ERR ? ERR : wclrtoeol((win)))
#define delete_part_line(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

/* save cursor position */
#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
/* return cursor to previously saved 
    position */
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

/* text styles */
typedef enum {
    NORMAL, 
    BOLD, 
    STANDOUT, 
    DIM, 
    ITALIC,
    ATTR_COUNT
} Attributes;

/* a container for UIWindow type */
typedef struct WindowManager WindowManager;

/* the text to be printed is divided into
    the following tokens:
    "<timestamp> <separator> <origin> <content>"
    each token can be individually formated with
    the color and the style of the format field */
typedef struct PrintTokens PrintTokens;

/* a function pointer to a function that prints 
    complex chars */
typedef void(*PrintFunc)(WindowManager *windowManager, cchar_t *string, int size);

WindowManager * create_windows(int sbMultiplier);
void delete_windows(WindowManager *);

PrintTokens * create_print_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format);
void delete_print_tokens(PrintTokens *printTokens);

/* set ncurses modes and options */
void set_windows_options(WindowManager *windowManager);

/* initialize ncurses colors */
void init_colors(int useColor);

/* set default layout, including the title,
    the borders and the messages */
void init_ui(WindowManager *windowManager, int useColor);

/* print a complex string */
void printstr(PrintTokens *printTokens, WindowManager *windowManager);

/* convert a regular string to a cchar_t
    representation and return the number
    of converted characters */
int string_to_complex_string(cchar_t *buffer, int size, const char *string, uint32_t format);

/* count cchar_t characters */
int count_complex_chars(cchar_t *string);

/* display a list of available commands */
void display_commands(WindowManager *windowManager, const CommandInfo *commands, int count);

/* display usage instructions for a command */
void display_usage(WindowManager *windowManager, const CommandInfo *command);

/* display app response to user commands*/
void display_response(WindowManager *windowManager, const char *response, ...);

/* display settings values */
void display_settings(WindowManager *windowManager);

/* display current time */
void display_time(WindowManager *windowManager);

/* display status information */
void display_status(WindowManager *windowManager, const char *status, ...);

/* display message received from the server */
void display_server_message(const char *string, void *arg);

/* reload and adjust UI elements after resize */
void resize_ui(WindowManager *windowManager, int useColors);

UIWindow * get_titlewin(WindowManager *windowManager);
UIWindow * get_chatwin(WindowManager *windowManager);
UIWindow * get_statuswin(WindowManager *windowManager);
UIWindow * get_inputwin(WindowManager *windowManager);

PrintFunc get_print_function(void);
void set_print_function(PrintFunc pf);

#endif