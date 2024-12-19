#ifndef PRINT_UTILS
#define PRINT_UTILS

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

typedef enum {
    NORMAL, 
    BOLD, 
    STANDOUT, 
    DIM, 
    ITALIC,
    ATTR_COUNT
} Attributes;

#define KEY_NEWLINE '\n'

#define BLOCK_CHAR L"\u2588"     

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
#define encode_text_format(hex1, hex2, value) ((value) >> (((hex2) - 1) * BITS_PER_HEX) ) | (((value) >> ((hex1) * BITS_PER_HEX)) & 0xF)

#define FORMAT_MASK_SEP 0x00F00F
#define FORMAT_MASK_ORG 0x0F00F0
#define FORMAT_MASK_CNT 0xF00F00

#define WHITE 0x00
#define RED 0x01
#define BLUE 0x02
#define MAGENTA 0x03
#define CYAN 0x04
#define CYAN_REV 0x5
#define RED_CYAN 0x6

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

#define delete_line(win) (wmove((win), 0, 0) == ERR ? ERR : wclrtoeol((win)))
#define delete_part_line(win, x) (wmove((win), 0, x) == ERR ? ERR : wclrtoeol((win)))

#define save_cursor(win, lasty, lastx) getyx(win, lasty, lastx)
#define restore_cursor(win, lasty, lastx) wmove(win, lasty, lastx)

#define add_newline(win) waddch(win, '\n')

/* convert a regular string to a cchar_t
    representation and return the number
    of converted characters */
int str_to_complex_str(cchar_t *buffer, int size, const char *string, uint32_t format);

/* count cchar_t characters */
int count_complex_chars(cchar_t *string);

#endif