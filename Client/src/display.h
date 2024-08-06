#ifndef DISPLAY_H
#define DISPLAY_H

#include "scrollback.h"
#include "command_parser.h"

#include <ncursesw/curses.h>

/*  
    Backspace is mapped to '^?' in terminfo and erase is mapped to '^?' 
    in termios. Ncurses uses 0407 (263) as KEY_BACKSPACE (defined in curses.h). 
    
    Additional constants KEY_BSPACE and KEY_ENTER were defined to handle 
    ASCII values returned by ncurses character input functions.
 */

#define KEY_SPACE ' '
#define KEY_BSPACE 127
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
#define delete_line_from_pos(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

WINDOW* get_mainwin(void);
WINDOW* get_chatwin(void);
WINDOW* get_statuswin(void);
WINDOW* get_inputwin(void);
Scrollback* get_scrollback(void);
LineEditor* get_line_editor(void);

void init_curses(void);
void init_windows(void);
void delete_windows(void);
void create_buffers(void);
void delete_buffers(void);  
void create_layout(void);
void init_colors(void);

void printmsg(uint8_t opt, const char *sep, const char *org, const char *msg, uint32_t attr);
int convert_to_cchar(cchar_t *line, const char *string, uint32_t attr);

void print_commands(const CommandInfo *cmd);
void print_usage(CommandInfo cmd);
void print_info(const char *info, ...);
void print_status(const char *status, ...);

void handle_resize(void);
void draw_window_borders(void);

#endif