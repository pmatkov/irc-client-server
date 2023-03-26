#define _XOPEN_SOURCE_EXTENDED 1
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <signal.h>
#include <ncursesw/curses.h>            // wide characters support

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
* Try to avoid using the global variables LINES and COLS. Use getmaxyx() on the stdscr context instead.
* SELECT !!!
* Free !!! - realloc provjera? 
* Server - Deamon?? (p. 1240 Kerrisk)

*/

int main(void)
{

     Command cmdLookup[] = {
        {HELP, "/help", {
            "", "", ""}},

        {CONNECT, "/connect", {
        "Syntax:        connect <adddress> [<port>]",
        "Description:   connects to the server at the specified address and port.",
        "Example:       /connect 127.0.0.1 49152. Allowed ports: above 49151. Default port: 50100"}},

        {DISCONNECT, "/disconnect", {
        "Syntax:        disconnect [<msg>]",
        "Description:   disconnects from the server with optional message.",
        "Example:       /disconnect Done for today!" }},

        {JOIN, "/join", {
        "Syntax:        join <channel>",
        "Description:   joins the specified channel.",
        "Example:       /join #general" }},

        {MSG, "/msg", {
        "Syntax:        msg <channel> <message>",
        "Description:   sends message to a channel.",
        "Example:       /msg #general I like pointers." }},

        {QUIT, "/quit", {
        "Syntax:        quit",
        "Description:   terminates the application",
        "Example:       /quit" }},
    };

    const int NUM_COMMANDS = sizeof(cmdLookup)/ sizeof(Command);

    setlocale(LC_CTYPE, "");        // sets default locale determined by the system's environment variables

    initscr();

     /* Disables line buffering, but control characters like CTRL+C
      * are still interpreted by the terminal driver.
      */
    cbreak();           
    noecho();
    start_color();

    init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(RED, COLOR_RED, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(CYAN_REV, COLOR_BLACK, COLOR_CYAN);

    WINDOW *topWin = newwin(LINES-1, COLS, 0, 0);       // window for app notifications and chat messages 
    WINDOW *bottomWin = newwin(1, COLS, LINES-1, 0);    // window for user input
                    
    keypad(bottomWin, TRUE);                            // enable detection of arrow keys

    wsetscrreg(topWin, 1, AVAILABLE_LNS);               // set scrolling region
    scrollok(topWin, TRUE);                             // enable scrolling
    idlok(topWin, TRUE);                                // use hardware insert/ delete line 

    /* FirstLn and lastLn are first and last lines of the 
     * available window area. LnsUpshifted represents currently
     + used scrollback, ie. how many lines have moved up beyond 
     * the visible window area.
     */
    PrintData wData = {.firstLn = 1, .lastLn = AVAILABLE_LNS, .printedLns = 0, .lnsUpshifted = 0}; 
    PrintData *printData = &wData;

    setup_windows(topWin, bottomWin, printData);

    char *input = (char*) calloc(COLS+1, sizeof(char));

    int key, x, y, charsPrinted = 0;
 
    while (1) {

        getyx(bottomWin, y, x);

        switch(key = wgetch(bottomWin)) {

            case KEY_LEFT:
                if (x > PROMPT_SIZE) {
                    wmove(bottomWin, y, --x);
                }
                break;

            case KEY_RIGHT:
                if (x <= charsPrinted + 1) {
                    wmove(bottomWin, y, ++x);
                }
                break;

            case KEY_UP:
                if (printData->lnsUpshifted) {
                    wscrl(topWin, -1);
                    println_from_buff(topWin, bottomWin, printData->firstLn, printData->firstLn + printData->lnsUpshifted - 1);
                    printData->lnsUpshifted--;
                }
                break;

            case KEY_DOWN:
                if (AVAILABLE_LNS + printData->lnsUpshifted < printData->printedLns) {
                    wscrl(topWin, 1);
                    println_from_buff(topWin, bottomWin, printData->lastLn, printData->lastLn + printData->lnsUpshifted + 1);
                    printData->lnsUpshifted++;
                }
                break;

            case KEY_PPAGE:
                if (printData->lnsUpshifted) {
                    int shift = (printData->lnsUpshifted >= AVAILABLE_LNS) ? AVAILABLE_LNS : printData->lnsUpshifted;
                    wscrl(topWin, -shift);
                    for (int i = 0; i < shift; i++) {
                        println_from_buff(topWin, bottomWin, printData->firstLn + i, printData->firstLn + printData->lnsUpshifted - shift + i);
                    }
                    printData->lnsUpshifted -= shift;
                }
                break;

            case KEY_NPAGE:
                if (AVAILABLE_LNS + printData->lnsUpshifted < printData->printedLns) {
                    int shift = (printData->printedLns - AVAILABLE_LNS - printData->lnsUpshifted >= AVAILABLE_LNS) ? AVAILABLE_LNS : printData->printedLns - AVAILABLE_LNS - printData->lnsUpshifted;
                    wscrl(topWin, +shift);
                    for (int i = 0; i < shift; i++) {
                        println_from_buff(topWin, bottomWin, printData->lastLn - shift + 1 + i, printData->lastLn + printData->lnsUpshifted + 1 + i);
                    }
                    printData->lnsUpshifted += shift;
                }
                break;

            case KEY_SPACE: 
                if (x < COLS-1 && charsPrinted < COLS-1)
                    add_space(bottomWin, input, y, x, &charsPrinted);
                break;

            case KEY_BSPACE:                    // backspace key
                if (x > PROMPT_SIZE) {
                    shift_chars(input, --x, &charsPrinted);
                    wmove(bottomWin, y, x);
                    wdelch(bottomWin);
                }
                break;
        
            case KEY_DC:                        // delete key
                if (x <= charsPrinted + 1) {
                    shift_chars(input, x, &charsPrinted);
                    wdelch(bottomWin);
                }
                break;

            case KEY_HOME:
                wmove(bottomWin, y, PROMPT_SIZE);
                break;

            case KEY_END:
                wmove(bottomWin, y, charsPrinted + PROMPT_SIZE);
                break;

            case KEY_IC:                    // insert key
                break;

            case KEY_NEWLINE:

                input[charsPrinted] = '\0';

                delete_line(bottomWin, y);
                wrefresh(bottomWin);

                if (charsPrinted)
                    validate_input(topWin, bottomWin, printData, input, cmdLookup, NUM_COMMANDS);

                charsPrinted = 0;
                break;

            // KEY_RESIZE is returned when the SIGWINCH signal has been detected
            case KEY_RESIZE:   
                handle_resize(topWin);

                break;

            default:
                if (x < COLS-1 && charsPrinted < COLS-1) {
                    waddch(bottomWin, key);
    
                    if (x == charsPrinted + PROMPT_SIZE){
                        input[charsPrinted++] = key;
                    }
                    else {
                        input[x-PROMPT_SIZE] = key;
                    }
                }

        }
    }

    endwin();

	return 0;
}

