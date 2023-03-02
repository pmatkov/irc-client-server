#define _XOPEN_SOURCE_EXTENDED 1
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
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
  A_NORMAL        Normal display (no highlight)
        A_STANDOUT      Best highlighting mode of the terminal.
        A_UNDERLINE     Underlining
        A_REVERSE       Reverse video
        A_BLINK         Blinking
        A_DIM           Half bright
        A_BOLD          Extra bright or bold
        A_PROTECT       Protected mode
        A_INVIS         Invisible or blank mode
        A_ALTCHARSET    Alternate character set
        A_CHARTEXT      Bit-mask to extract a character
        COLOR_PAIR(n)   Color-pair number n

         whline(win, ACS_BLOCK               , ncolumns);
*/

int main(void)
{
    
     Command cmdLookup[] = {
        {HELP, "/help", {
            "", "", ""}},

        {CONNECT, "/connect", {
        "Syntax: connect <adddress> [<port>]",
        "Description: sconnects to the server at the specified address and port.",
        "Example: /connect 127.0.0.1 49152. Allowed ports: above 49151. Default port: 50100"}},

        {DISCONNECT, "/disconnect", {
        "Syntax: disconnect [<msg>]"
        "Description: disconnects from the server with optional message.",
        "Example: /disconnect Done for today" }},

        {JOIN, "/join", {
        "Syntax: join <channel>",
        "Description: joins the specified channel.",
        "Example: /join #general" }},

        {MSG, "/msg", {
        "Syntax: msg <channel> <message>",
        "Description: sends message to a channel.",
        "Example: /msg #general Where is everybody?" }},

        {QUIT, "/quit", {
        "Syntax: quit",
        "Description: terminates the application",
        "Example: /quit" }},
    };

    const int NUM_COMMANDS = sizeof(cmdLookup)/ sizeof(Command);

    setlocale(LC_CTYPE, "");        // sets default locale determined by the system's environment variables

    initscr();

     /* Disables line buffering, but control characters like 
      * CTRL+C are still interpreted by the terminal driver
      */
    cbreak();           
    noecho();
    start_color();

    init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(CYAN_REV, COLOR_BLACK, COLOR_CYAN);

    WINDOW *topWin = newwin(LINES-1, COLS, 0, 0);        // window for app notifications and chat messages 
    WINDOW *bottomWin = newwin(1, COLS, LINES-1, 0);     // window for user input
                    
    keypad(bottomWin, TRUE);                            // enable detection of arrow keys

    wsetscrreg(topWin, 1, AVAILABLE_LNS);               // set scrolling region
    scrollok(topWin, TRUE);                             // enable scrolling
    idlok(topWin, TRUE);                                // use hardware insert/ delete line 

    PrintData wData = {.firstLn = 1, .lastLn = AVAILABLE_LNS, .printedLns = 0, .lnsUpshifted = 0}; // shift is difference between firstln and actually displayed first line number
    PrintData *printData = &wData;

    setup_windows(topWin, bottomWin, printData);

    char *input = (char*) calloc(COLS+1, sizeof(char));

    int key, x, y, charsPrinted = 0;
 
    while (1) {

        getyx(bottomWin, y, x);
        // getmaxyx(stdscr, rows, cols);

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
                    print_saved_line(topWin, bottomWin, printData->firstLn + printData->lnsUpshifted - 1, printData->firstLn);
                    printData->lnsUpshifted--;
                }
                break;

            case KEY_DOWN:
                if (AVAILABLE_LNS + printData->lnsUpshifted < printData->printedLns) {
                    wscrl(topWin, 1);
                    print_saved_line(topWin, bottomWin, printData->lastLn + printData->lnsUpshifted + 2, printData->lastLn);
                    printData->lnsUpshifted++;
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

            case KEY_NPAGE:                 // disabled for now
            case KEY_PPAGE:
            case KEY_IC:                    // insert key
                break;

            case KEY_NEWLINE:

                input[charsPrinted] = '\0';
                charsPrinted = 0;
        
                validate_input(topWin, printData, input, cmdLookup, NUM_COMMANDS);
                delete_line(bottomWin, y);
                break;

            /* KEY_RESIZE is returned when the SIGWINCH signal has been detected. 
            * It alerts an application that the screen size has changed and that
            * it should repaint special features such as pads that cannot be done
            * automatically.
            */
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

