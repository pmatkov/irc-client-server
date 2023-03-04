#ifndef UI_H
#define UI_H

#include <ncursesw/curses.h>

/* My terminfo defines 'kbs' as '^H' and my terminal settings define 'erase' as '^?'. 
 * When I press backspace, ncurses doesn't translate it to KEY_BACKSPACE (defined in 
 * curses.h). Instead, it returns ASCII 127 (^?). As a workaround, I defined KEY_BSPACE
 * to represent the returned value. I also defined KEY_NEWLINE because KEY_ENTER is not
 * used for regular enter key (apparently), but the one one a numeric keyboard.
 */

#define KEY_SPACE ' '
#define KEY_BSPACE 127
#define KEY_NEWLINE '\n'
#define BLOCK_CHAR L"\u2588"     // Unicode full block character

#define PROMPT "> "
#define PROMPT_SIZE 2
#define SPACE "    "

#define WHITE 0
#define RED 1
#define BLUE 2
#define MAGENTA 3
#define CYAN 4
#define CYAN_REV 5

#define AVAILABLE_LNS (LINES-3)
#define USAGE_LNS 3

#define delete_line(win, y)	(wmove((win),(y),(PROMPT_SIZE)) == ERR ? ERR : wclrtoeol((win)))
#define print_empty(topWin, bottomWin, printData) print_text_ext(topWin, bottomWin, printData, "", NULL, NULL, 0, 0, 0);

#define get_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    JOIN,
    MSG,
    QUIT
} CommandEnum;

typedef struct {
    CommandEnum commandEnum;
    const char *name;
    const char *usage[USAGE_LNS];       
} Command;

typedef struct {
    int firstLn;
    int lastLn;
    int printedLns;
    int lnsUpshifted;
} PrintData;

void setup_windows(WINDOW *, WINDOW *, PrintData *);
void println_from_buff(WINDOW *, WINDOW *, int, int);
void shift_chars(char *, int, int *);
void add_space(WINDOW *, char *, int, int, int *);
void validate_input(WINDOW *, WINDOW *, PrintData *, char *, Command *, int);
void handle_resize(WINDOW *);

#endif