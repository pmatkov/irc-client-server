#define _XOPEN_SOURCE_EXTENDED 1
#include "ui.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <wchar.h>
#include <ncursesw/curses.h>

#define MAX_CHARS 512
#define MAX_LINES (AVAILABLE_LNS)*10
#define MAX_ARGS 3

void allocateBuff(PrintData *);

void print_text(WINDOW*, const char *, int);
void print_text_std(WINDOW *, int, int, char *, int);
void print_text_ext(WINDOW *, PrintData *printData, const char *, const char *, const char *, int, int, int);

void get_time(char *, int);
Command * get_command(char *, Command *, int, int);
void display_commands(WINDOW*, PrintData *printData, Command *, int);
void display_usage(WINDOW *, PrintData *printData, Command);
void failed(const char *);

static cchar_t **lineBuffer = NULL;

void setup_windows(WINDOW *topWin, WINDOW *bottomWin, PrintData *printData) {

    allocateBuff(printData);

    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    mvwhline_set(topWin, 0, 0, &block, COLS);
    mvwhline_set(topWin, LINES-2, 0, &block, COLS);

    print_text_std(topWin, 0, 0, " Chat", COLOR_PAIR(CYAN_REV));
    print_text_ext(topWin, printData, "", " ## ", "Chat v 0.1", 0, COLOR_PAIR(MAGENTA), 0);
    print_empty(topWin, printData);
    print_text_ext(topWin, printData, "", " ## ", "Type /help for a list of available commands.", 0, COLOR_PAIR(MAGENTA), 0);


    for (int i = 0; i < 50; i++)
    {
        char arr[2] = {0};
        arr[0] = '0' + i;
        print_text_ext(topWin, printData, "", " ## ", arr, 0, COLOR_PAIR(MAGENTA), 0);
    }

    mvwprintw(bottomWin, 0, 0, PROMPT);

    wrefresh(topWin);
    wrefresh(bottomWin);
}

void allocateBuff(PrintData *printData){

    lineBuffer = (cchar_t **) realloc(lineBuffer, (printData->printedLns + AVAILABLE_LNS) * sizeof(cchar_t*));
    if (lineBuffer == NULL)
        failed("Error allocating memory.");

    for (int i = printData->printedLns; i < printData->printedLns + AVAILABLE_LNS; i++){
        lineBuffer[i] = (cchar_t *) calloc(MAX_CHARS + 1, sizeof(cchar_t));
        if (lineBuffer[i] == NULL)
            failed("Error allocating memory.");

    }
}

void print_text(WINDOW *win, const char *string, int color) {
    wattron(win, color);
    wprintw(win, string);
    wattroff(win, color);
}

void print_text_std(WINDOW *win, int y, int x, char *string, int color) {

    wattron(win, color);
    mvwprintw(win, y, x, string);
    wattroff(win, color);
    wmove(win, ++y, 0);
}

void print_text_ext(WINDOW *win, PrintData *printData, const char *time, const char *separator, const char *string, int tmColor, int sepColor, int strColor) {

    int lastx, lasty, len = 0;

    if (time != NULL) {

        char buffer[] = "hh:mm";
        get_time(buffer, sizeof(buffer));

        print_text(win,buffer, (tmColor == 0) ? COLOR_PAIR(WHITE) : tmColor);
        len += strlen(buffer);
    }

    if (separator != NULL) {
        print_text(win, separator, (sepColor == 0) ? COLOR_PAIR(WHITE) : sepColor);
        len += strlen(separator);
    }

    if (string != NULL) {
        print_text(win, string, (strColor == 0) ? COLOR_PAIR(WHITE) : strColor);
        len += strlen(string);
    }
    printData->printedLns++;

    if (printData->printedLns % AVAILABLE_LNS == 0 && printData->printedLns < MAX_LINES) {
        allocateBuff(printData);
    }

    save_cursor(win, lasty, lastx);
    mvwin_wchnstr(win, lasty, 0, lineBuffer[printData->printedLns-1], len);
    restore_cursor(win, lasty, lastx);

    wprintw(win, "\n");

    if (printData->printedLns > AVAILABLE_LNS) 
        printData->lnsUpshifted++;
    
}


void print_saved_line(WINDOW *topWin, WINDOW *bottomWin, int line, int y) {

    int lastx, lasty;

    save_cursor(bottomWin, lasty, lastx);
    wmove(topWin, y, 0);

    for (int i = 0; wcslen(lineBuffer[line-1][i].chars); i++) {
        wadd_wch(topWin, &lineBuffer[line-1][i]);
    }

    restore_cursor(bottomWin, lasty, lastx);
    wrefresh(topWin);

}

void get_time(char *timeBuffer, int n) {

    time_t rawtime;
    struct tm *loctime;

    time(&rawtime);
    loctime = localtime(&rawtime);

    strftime(timeBuffer, n, "%H:%M", loctime);
}

void shift_chars(char *input, int x, int *charsPrinted) {

    for (int i = x; i <= *charsPrinted + 1; i++) {
        
        input[i-PROMPT_SIZE] = input[i-PROMPT_SIZE+1];
    }

    input[*charsPrinted-PROMPT_SIZE+1] = '\0';
    --*charsPrinted;
}

void add_space(WINDOW *win, char *input, int y, int x, int *charsPrinted) {

    for (int i = *charsPrinted + 1; i >= x; i--) {

        char key = mvwinch(win, y, i);
        mvwaddch(win, y, i+1, key);
        input[i-PROMPT_SIZE+1] = input[i-PROMPT_SIZE];
    }

    mvwaddch(win, y, x, ' ');

    input[x-PROMPT_SIZE] = ' ';
    ++*charsPrinted;
    wmove(win, y, x + 1);

}

void validate_input(WINDOW *win, PrintData *printData, char *input, Command *cmdLookup, int numCommands) {

    int argCount = 0, unknownCmd = 0;
    char *inputCopy, *token;
    char *allTokens[MAX_ARGS] = {NULL};
    Command *command, *argument;

    inputCopy = strdup(input);

    while ((token = strtok_r(inputCopy, " ", &inputCopy)) && argCount < MAX_ARGS)
        allTokens[argCount++] = token;

    if ((command = get_command(allTokens[0], cmdLookup, numCommands, 1))) {

        switch (command->commandEnum) {
            case HELP: 
                if (argCount == 1)
                    display_commands(win, printData, cmdLookup, numCommands);
                else if (argCount == 2 && (argument = get_command(allTokens[1], cmdLookup, numCommands, 2)) && argument->commandEnum != HELP){
                    display_usage(win, printData, *argument);
                }
                else
                    unknownCmd = 1;
                break;
            case CONNECT: 
                // if (argCount == 2 || argCount == 3)
                //     // validate_address_port(); 
                // else
                //     invalidCommand = 1;
                break;
            case DISCONNECT: 
            case JOIN: 
            case MSG: 
            case QUIT: 
                break;
        }
    }
    else {
      unknownCmd = 1;
    }

    if (unknownCmd) {
        mvwprintw(win, AVAILABLE_LNS, 0, "Unknown command: %s", input);
        wrefresh(win);

    }

}

Command * get_command(char *input, Command *cmdLookup, int numCommands, int lookupType) {

    /* skip inital '/' if command is argument to /help (eg. /help join instead of /help /join) */
    int skip = lookupType == 1 ? 0 : 1;     

    for (int i = 0; i < numCommands - 1; i++) {
        if (strcmp(input, &cmdLookup[i].name[skip]) == 0)
            return &cmdLookup[i];
    }

    return NULL;
}


void display_commands(WINDOW *win, PrintData *printData, Command *cmdLookup, int n) {

    print_empty(win, printData);
    print_text_ext(win, printData, "", " ** ", "Commands:", 0, COLOR_PAIR(CYAN), 0);

    for (int i = 1; i < n; i++) {
        print_text_ext(win, printData, "", SPACE, &cmdLookup[i].name[1], 0, 0, 0);
    }

    print_text_ext(win, printData, "", SPACE, "For info about a command type /help <command name>.", 0, 0, 0);
  
    wrefresh(win);
}

void display_usage(WINDOW *win, PrintData *printData, Command command) {

    print_empty(win, printData);

    print_text_ext(win, printData, "", SPACE, command.usage[0], 0, 0, 0);
    print_text_ext(win, printData, "", SPACE, command.usage[1], 0, 0, 0);
    print_text_ext(win, printData, "", SPACE, command.usage[2], 0, 0, 0);

    wrefresh(win);
}


void handle_resize(WINDOW *win) {

    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    mvwhline_set(win, 0, 0, &block, COLS); 
    mvwhline_set(win, LINES-2, 0, &block, COLS);

    wrefresh(win);
    
}

void failed(const char *msg)
{
    endwin();
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}


