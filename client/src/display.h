#ifndef DISPLAY_H
#define DISPLAY_H

/* activates ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include "scrollback.h"
#include "line_editor.h"
#include "../../shared/src/command.h"

#include <ncursesw/curses.h>

#define KEY_NEWLINE '\n'

/* defines Unicode block character which
    is used to create UI borders */
#define BLOCK_CHAR L"\u2588"

/* defines input prompt */
#define PROMPT "> "
#define PROMPT_SIZE 2

#define SPACE "    "
#define SPACE_2 "        "

#define WHITE 0x00
#define RED 0x01
#define BLUE 0x02
#define MAGENTA 0x03
#define CYAN 0x04
#define CYAN_REV 0x5

/* used to create a bit field of colors 
    and attributes of tokenized text */
#define COLOR_SEP(color) ((color) << (0))
#define COLOR_ORG(color) ((color) << (4))
#define COLOR_MSG(color) ((color) << (8))

#define ATTR_SEP(attr) ((attr) << (12))
#define ATTR_ORG(attr) ((attr) << (16))
#define ATTR_MSG(attr) ((attr) << (20))

#define get_wwidth(win) getmaxx(win)
#define get_wheight(win) getmaxy(win)

/* deletes complete or partial line on 
    the screen (partial deletes the text
    from the cursor until the end of the 
    line)*/
#define delete_line(win) (wmove((win), 0, 0) == ERR ? ERR : wclrtoeol((win)))
#define delete_part_line(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

/* saves cursor position and, when required,
    returns it back to that position */
#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

/* defines ncurses text attributes */
typedef enum {
    NORMAL, 
    BOLD, 
    STANDOUT, 
    DIM, 
    ITALIC,
    ATTR_COUNT
} Attributes;

/* contains references to ncurses window 
	structures */
typedef struct WindowManager WindowManager;

/* PrintTokens represents tokenized text which
    will be displayed on the screen. this 
    text is divided into the following tokens:
    "<timestamp> <separator> <origin> <message>".
    each token can be formatted with different 
    colors and attributes */
typedef struct PrintTokens PrintTokens;

WindowManager * create_windows(void);
void delete_windows(WindowManager *);

PrintTokens * create_print_tokens(int useTimestamp, const char *separator, const char *origin, const char *message);
void delete_print_tokens(PrintTokens *printTokens);

/* sets ncurses modes and options */
void set_windows_options(WindowManager *windowManager);

/* initializes ncurses color mode */
void init_colors(int useColor);

/* displays initial visual elements of the UI, 
    including the title, the borders and the 
    standard messages */
void create_layout(WindowManager *windowManager, Scrollback *scrollback, int useColor);

/* prints a formatted message. attributes 
    is a bit field of attributes and colors
    used by printTokens */
void printmsg(Scrollback *scrollback, PrintTokens *printTokens, uint32_t attributes);

/* converts a regular string to ncurses 
    cchar_t representation and returns
    the number of converted chars */
int string_to_complex_string(const char *string, cchar_t *buffer, int len, uint32_t attributes);

/* counts cchar_t chars in a complex 
    string  */
int count_complex_chars(cchar_t *string);

/* displays a list of available commands */
void display_commands(Scrollback *scrollback, const Command *commands, int count);

/* displays usage instructions for a
    command */
void display_usage(Scrollback *scrollback, const Command *command);

/* displays app response to user 
    commands*/
void display_response(Scrollback *scrollback, const char *response, ...);

/* displays settings values */
void display_settings(Scrollback *scrollback);

/* displays status information in 
    the status window */
void display_status(WindowManager *windowManager, const char *status, ...);

/* resizes ncurses windows and recreates 
    layout after window resize event */
void repaint_ui(WindowManager *windowManager, Scrollback *scrollback, LineEditor *lnEditor, int useColors);

WINDOW *get_chatwin(WindowManager *windowManager);
WINDOW *get_statuswin(WindowManager *windowManager);
WINDOW *get_inputwin(WindowManager *windowManager);

#endif