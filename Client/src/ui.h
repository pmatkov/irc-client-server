#ifndef UI_H
#define UI_H

#include <ncursesw/curses.h>

/* My terminal description (terminfo) is not consistent with terminal driver's
 * settings (stty) in respect to backspace. Terminfo defines 'kbs' as '^H' and
 * terminal settings defines 'erase' as '^?'. In this situation, when I press 
 * backspace, ncurses doesn't translate it to KEY_BACKSPACE which is defined in 
 * curses.h, but returns ASCII 127 (^?). As a workaround, I have defined KEY_BSPACE
 * to represent the returned value. 
 */

#define KEY_SPACE ' '
#define KEY_BSPACE 127
#define KEY_NEWLINE '\n'
#define BLOCK_CHAR L"\u2588"     // Unicode full block character

#define PROMPT "> "
#define PROMPT_SIZE 2
#define SPACE "    "
#define CUR_STAY 0
#define CUR_ADV 1

#define WHITE 0
#define RED 1
#define BLUE 2
#define MAGENTA 3
#define CYAN 4
#define CYAN_REV 5

#define INPUT_LINES 1
#define BORDER_HEIGHT 1
#define AVAILABLE_LNS (LINES-3)

#define delete_line(win, y)	(wmove((win),(y),(PROMPT_SIZE)) == ERR ? ERR : wclrtoeol((win)))
// #define print_empty(win, printData, y) print_texttm(win, printData, y, 0, "", "\n", NULL, 0, 0, 0);
#define print_empty(win, printData) print_text_ext(win, printData, "", NULL, NULL, 0, 0, 0);

#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
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
    const char *usage[3];       
} Command;

typedef struct {
    int firstLn;
    int lastLn;
    int printedLns;
    int lnsUpshifted;
} PrintData;

void setup_windows(WINDOW *, WINDOW *, PrintData *);
void print_saved_line(WINDOW *, WINDOW *, int, int);
void shift_chars(char *, int, int *);
void add_space(WINDOW *, char *, int, int, int *);

void validate_input(WINDOW *, PrintData *, char *, Command *, int);
void handle_resize(WINDOW *);

#endif