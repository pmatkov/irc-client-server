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

/* Unicode block character used to 
    create window borders */
#define BLOCK_CHAR L"\u2588"

/* the "input window" prompt */
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

/* macros that create a bit field of colors and
    attributes for formatting complex characters */
#define COLOR_SEP(color) ((color) << (0))
#define COLOR_ORG(color) ((color) << (4))
#define COLOR_CNT(color) ((color) << (8))

#define ATTR_SEP(attr) ((attr) << (12))
#define ATTR_ORG(attr) ((attr) << (16))
#define ATTR_CNT(attr) ((attr) << (20))

#define get_wwidth(win) getmaxx(win)
#define get_wheight(win) getmaxy(win)

/* delete complete or partial line on the
    screen (partial delete deletes the text
    from the cursor until the end of the 
    line) */
#define delete_line(win) (wmove((win), 0, 0) == ERR ? ERR : wclrtoeol((win)))
#define delete_part_line(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

/* save cursor position and, when required,
    return it back to that position */
#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

/* define enums for ncurses text attributes */
typedef enum {
    NORMAL, 
    BOLD, 
    STANDOUT, 
    DIM, 
    ITALIC,
    ATTR_COUNT
} Attributes;

/* contains references to window
    structures */
typedef struct WindowManager WindowManager;

/* contains tokens with formatting attributes
    for the text to be printed. the printed text 
    consists of the following tokens:
    "<timestamp> <separator> <origin> <content>" */
typedef struct PrintTokens PrintTokens;

/* pointer to a function which prints complex
    chars to a window */
typedef void(*PrintFunc)(WindowManager *windowManager, cchar_t *string, int size);

WindowManager * create_windows(int sbMultiplier);
void delete_windows(WindowManager *);

PrintTokens * create_print_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format);
void delete_print_tokens(PrintTokens *printTokens);

/* set ncurses modes and options */
void set_windows_options(WindowManager *windowManager);

/* initialize ncurses color mode */
void init_colors(int useColor);

/* create user interface with default layout
    and elements, including the title, the
    borders and the welcome messages */
void init_ui(WindowManager *windowManager, int useColor);

/* prepare and print a formatted string from 
    complex characters */
void printstr(PrintTokens *printTokens, WindowManager *windowManager);

/* convert a regular string to a cchar_t
    representation and return the number
    of converted characters */
int string_to_complex_string(cchar_t *buffer, int size, const char *string, uint32_t format);

/* count cchar_t characters */
int count_complex_chars(cchar_t *string);

/* display a list of available commands */
void display_commands(WindowManager *windowManager, const Command *commands, int count);

/* display usage instructions for a command */
void display_usage(WindowManager *windowManager, const Command *command);

/* display app response to user commands*/
void display_response(WindowManager *windowManager, const char *response, ...);

/* display settings values */
void display_settings(WindowManager *windowManager);

/* displays status information */
void display_status(WindowManager *windowManager, const char *status, ...);

/* reload and adjust UI elements and content
    on terminal resize */
void resize_ui(WindowManager *windowManager, int useColors);

UIWindow * get_titlewin(WindowManager *windowManager);
UIWindow * get_chatwin(WindowManager *windowManager);
UIWindow * get_statuswin(WindowManager *windowManager);
UIWindow * get_inputwin(WindowManager *windowManager);

PrintFunc get_print_function(void);
void set_print_function(PrintFunc pf);

#endif