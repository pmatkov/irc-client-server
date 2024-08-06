#include "main.h"
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <ncursesw/curses.h> 


/* Todo:
*  1. setup servera: create socket, bind, listen
*  2. prihvat zahtjeva za uspostavu veze: novi socket za uspostavljenu vezu, dodati klijenta u listu spojenih klijenata
*  3. primanje i prosljedjivanje poruka klijentima
*  4. prekidi veze: brisanje klijenta iz liste spojenih klijenata i obavjestavanje ostalih klijenata 
*  da je klijent prekinuo vezu
* - povezana lista u kojoj ces se cuvati svi spojeni klijenti i njhovi socketi (mozda bolje hash tablica ili heap)
* - static functions
* waddch(win, 'X' | A_UNDERLINE | COLOR_PAIR(3));
* leaveok, doupdate
* SELECT !!!
* Server - Deamon?? (p. 1240 Kerrisk)

*/

#ifndef TEST

int main(void)
{

    init_curses();
    init_windows();
    create_buffers();
    create_layout();

    WINDOW *mainwin = get_mainwin();
    WINDOW *statuswin = get_statuswin();
    WINDOW *inputwin = get_inputwin();

    Scrollback *scrollback = get_scrollback();
    LineEditor *lnEditor = get_line_editor();

    if (mainwin != NULL || statuswin != NULL || \
    scrollback != NULL || lnEditor != NULL) {
        failed("Display structures aren't initialized");
    }

    int terminated = 0;

    while (!terminated) {

        int ch = wgetch(inputwin);

        switch (ch) {

            case KEY_LEFT:
                move_cursor_left(lnEditor);
                break;

            case KEY_RIGHT:
                move_cursor_right(lnEditor);
                break;

            case KEY_UP:
                scroll_line_up(scrollback);
                break;

            case KEY_DOWN:
                scroll_line_down(scrollback);
                break;

            case KEY_PPAGE:
                scroll_page_up(scrollback);
                break;

            case KEY_NPAGE:
                scroll_page_down(scrollback);
                break;

            case KEY_BACKSPACE:
                use_backspace(lnEditor);
                break;
        
            case KEY_DC:
                use_delete(lnEditor);
                break;

            case KEY_HOME:
                wmove(lnEditor->window, 0, PROMPT_SIZE);
                break;

            case KEY_END:
                wmove(lnEditor->window, 0, lnEditor->charCount + PROMPT_SIZE);
                break;

            // case KEY_IC:
            //     break;

            case KEY_NEWLINE:
                terminated = parse_input(scrollback, lnEditor);
                break;

            // SIGWINCH handler sends KEY_RESIZE on window resize
            case KEY_RESIZE:   
                handle_resize();
                break;

            default:
                add_char(lnEditor, ch);

        }
        wrefresh(lnEditor->window);
    }

    delete_windows();
    delete_buffers();

	return 0;
}

#endif

void failed(const char *msg, ...) {

    delete_windows();
    delete_buffers();

    // save error number before calling functions which could change it
    int errnosv = errno;

    // check for additional args
    if (strchr(msg, '%') == NULL) {
        fprintf(stderr, msg);
    } else { 
        va_list arglist;
        va_start(arglist, msg);

        vfprintf(stderr, msg, arglist);
        va_end(arglist);
    }

    if (errnosv) {
        fprintf(stderr, " (error code = %d: %s).\n", errnosv, strerror(errnosv));
    }
 
    exit(EXIT_FAILURE);
}






