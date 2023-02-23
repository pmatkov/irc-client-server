#ifndef UI_H
#define UI_H

#include <ncursesw/curses.h>

/* My terminal description (terminfo) is not consistent with terminal driver's
 * settings (stty) in respect to backspace. Terminfo defines 'kbs' as '^H' and terminal
 * settings defines 'erase' as '^?'. In this situation, when I press backspace,
 * ncurses doesn't translate it to KEY_BACKSPACE constant, which is defined in curses.h, 
 * but rather returns ASCII 127 (^?). As a workaround, I have defined KEY_BSPACE to represent
 * the returned value. 
 */

#define KEY_SPACE ' '
#define KEY_BSPACE 127
#define KEY_NEWLINE '\n'
#define KEY_TAB '\t'
#define BLOCK_CHAR L"\u2588"     // Unicode full block character

#define PROMPT "> "
#define PROMPT_SIZE 2

#define WHITE_BLACK 0
#define CYAN_BLACK 1
#define BLACK_CYAN 2
#define MAGENTA_BLACK 3
#define BLUE_BLACK 4

#define SET_COLOR(color) COLOR_PAIR(color)

#define delete_line(win, y)	(wmove((win),(y),(PROMPT_SIZE)) == ERR ? ERR : wclrtoeol((win)))

typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    JOIN,
    QUIT, 
    INVALID_COMMAND
} CommandEnum;

typedef struct {
    const char *commandString;
    CommandEnum commandEnum;
} Command;

void setup_windows(WINDOW *, WINDOW *, int *);
void initialize_colors(void);
void shift_chars(char *, int, int *);
void add_space(WINDOW *, char *, int, int, int *);

CommandEnum get_user_command(char *, Command *, int);
void display_help(WINDOW*, int *, Command *, int);
void handle_resize(WINDOW *, int, int);

#endif