#define _XOPEN_SOURCE_EXTENDED 1
#include "ui.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncursesw/curses.h>            // using wide chars

int main(void)
{

    setlocale(LC_CTYPE, "");

    initscr();
    cbreak();               // disables line buffering but control characters like CTRL+C are still interpreted by the terminal driver
    noecho();

    initialize_colors();

    int key, x, y, rows, cols, charsWritten = 0, currentLine = 1;

    WINDOW *topWin = newwin(LINES-1, COLS, 0, 0);        // top window for notifications and chat history 
    WINDOW *bottomWin = newwin(1, COLS, LINES-1, 0);     // bottom window for user input

    keypad(topWin, TRUE);                                // enable detection of arrow keys
    keypad(bottomWin, TRUE); 

    setup_windows(topWin, bottomWin, &currentLine);

    char *inputBuffer = (char*) calloc(COLS+1, sizeof(char));        // allocate buffer for one line of input

    Command commandLookup[] = {
        {"/help", HELP},
        {"/connect", CONNECT},
        {"/disconnect", DISCONNECT},
        {"/join", JOIN},
        {"/quit", QUIT},
        {"/invalid command", INVALID_COMMAND}
    };

    const int NUM_COMMANDS = sizeof(commandLookup)/ sizeof(Command);

    while (1) {

        getyx(bottomWin, y, x);
        getmaxyx(stdscr, rows, cols);
        key = wgetch(bottomWin);

        switch(key) {

            case KEY_LEFT:
                if (x > PROMPT_SIZE) {
                    wmove(bottomWin, y, --x);
                }
                break;

            case KEY_RIGHT:
                if (x <= charsWritten + 1) {
                    wmove(bottomWin, y, ++x);
                }
                break;

            case KEY_UP:
            case KEY_DOWN:
                break;

            case KEY_SPACE: 
                if (x < COLS-1 && charsWritten < COLS - 1)
                    add_space(bottomWin, inputBuffer, y, x, &charsWritten);
                break;

            case KEY_BSPACE:                    //backspace
                if (x > PROMPT_SIZE) {
                    shift_chars(inputBuffer, --x, &charsWritten);
                    wmove(bottomWin, y, x);
                    wdelch(bottomWin);
                }
                break;
        
            case KEY_DC:                        // delete
                if (x <= charsWritten + 1) {
                    shift_chars(inputBuffer, x, &charsWritten);
                    wdelch(bottomWin);
                }
                break;

            case KEY_HOME:
                wmove(bottomWin, y, PROMPT_SIZE);
                break;

            case KEY_END:
                wmove(bottomWin, y, charsWritten + PROMPT_SIZE);
                break;

            case KEY_TAB:
            case KEY_NPAGE:
            case KEY_PPAGE:
            case KEY_IC:
                break;

            case KEY_NEWLINE:

                inputBuffer[charsWritten] = '\0';
                charsWritten = 0;
        
                CommandEnum command = get_user_command(inputBuffer, commandLookup, NUM_COMMANDS);

                switch (command) {
                    case HELP: 
                        display_help(topWin, &currentLine, commandLookup, NUM_COMMANDS);
                        delete_line(bottomWin, y);
                        break;
                    case CONNECT: 
                    case DISCONNECT: 
                    case JOIN: 
                    case QUIT: 
                    case INVALID_COMMAND: 
                        delete_line(bottomWin, y);
                        break;
                }
                break;

            case KEY_RESIZE:
                handle_resize(topWin, cols);
                break;

            default:
                if (x < COLS-1 && charsWritten < COLS - 1) {
                    waddch(bottomWin, key);
    
                    if (x == charsWritten + PROMPT_SIZE){
                        inputBuffer[charsWritten++] = key;
                    }
                    else {
                        inputBuffer[x-PROMPT_SIZE] = key;
                    }
                }
        }

        wrefresh(topWin);
        wrefresh(bottomWin);
    }

    endwin();

	return 0;
}

