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

void allocate_memory(PrintData *);

void print_text(WINDOW*, const char *, int);
void print_text_std(WINDOW *, int, int, const char *, int);
void saveln_to_buff(int, int, const char *, int);
void print_text_ext(WINDOW *, WINDOW *, PrintData *printData, const char *, const char *, const char *, int, int, int);

void get_time(char *, int);
Command * get_command(char *, Command *, int, int);
void display_commands(WINDOW*, WINDOW*, PrintData *printData, Command *, int);
void display_usage(WINDOW *, WINDOW*, PrintData *printData, Command);
void failed(const char *);

static cchar_t **lineBuffer = NULL;
static wchar_t *msgBuffer = NULL;


void setup_windows(WINDOW *topWin, WINDOW *bottomWin, PrintData *printData) {

    msgBuffer = (wchar_t*) calloc(COLS+1, sizeof(wchar_t));
    if (msgBuffer == NULL)
        failed("Error allocating memory.");

    allocate_memory(printData);

    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    mvwhline_set(topWin, 0, 0, &block, COLS);
    mvwhline_set(topWin, LINES-2, 0, &block, COLS);

    print_text_std(topWin, 0, 0, " Chat", COLOR_PAIR(CYAN_REV));
    print_text_ext(topWin, bottomWin, printData, "", " ## ", "Chat v 0.1", 0, MAGENTA, 0);
    print_empty(topWin, bottomWin, printData);
    print_text_ext(topWin, bottomWin, printData, "", " ## ", "Type /help for a list of available commands.", 0, MAGENTA, 0);

    mvwprintw(bottomWin, 0, 0, PROMPT);

    wrefresh(topWin);
    wrefresh(bottomWin);
}

void allocate_memory(PrintData *printData){

    lineBuffer = (cchar_t **) realloc(lineBuffer, (printData->printedLns + AVAILABLE_LNS) * sizeof(cchar_t*));
    if (lineBuffer == NULL)
        failed("Error allocating memory.");

    for (int i = printData->printedLns; i < printData->printedLns + AVAILABLE_LNS; i++){
        lineBuffer[i] = (cchar_t *) calloc(MAX_CHARS + 1, sizeof(cchar_t));
        if (lineBuffer[i] == NULL)
            failed("Error allocating memory.");
    }
}

void print_text(WINDOW *win, const char *string, int attribute) {
    wattron(win, attribute);
    wprintw(win, string);
    wattroff(win, attribute);
}

void print_text_std(WINDOW *win, int y, int x, const char *string, int attribute) {

    wattron(win, attribute);
    mvwprintw(win, y, x, string);
    wattroff(win, attribute);
    wmove(win, ++y, 0);
}

void saveln_to_buff(int line, int column, const char *string, int attribute) {

    wchar_t *mPtr = msgBuffer;
    size_t mbslen;
    attr_t attr = 0;
    int color = 0;
 
    // this range represents colors from ui.h
    if (attribute >= 0 && attribute <= 5) 
        color = attribute;
    else
        attr = attribute;

    if ((mbslen = mbstowcs(NULL, string, 0)) == (size_t) -1)
        failed("Error getting length for wchar");

    if (mbstowcs(msgBuffer, string, mbslen + 1) == (size_t) -1)
        failed("Error converting char to wchar.");

    while (*mPtr)
        setcchar(&lineBuffer[line][column++], mPtr++, attr, color, NULL);
    
}


void print_text_ext(WINDOW *topWin, WINDOW *bottomWin, PrintData *printData, const char *time, const char *separator, const char *string, int tmAttribute, int sepAttribute, int strAttribute) {

    int column = 0;

    if (time != NULL) {

        char buffer[] = "hh:mm";
        get_time(buffer, sizeof(buffer));

        saveln_to_buff(printData->printedLns, column, buffer, tmAttribute);
        column += strlen(buffer);
    }

    if (separator != NULL) {
        saveln_to_buff(printData->printedLns, column, separator, sepAttribute);
        column += strlen(separator);
    }

    if (string != NULL) {
        saveln_to_buff(printData->printedLns, column, string, strAttribute);
        column += strlen(string);
    }

    printData->printedLns++;

    if (printData->printedLns % AVAILABLE_LNS == 0 && printData->printedLns < MAX_LINES) {
        allocate_memory(printData);
    }

    if (AVAILABLE_LNS + printData->lnsUpshifted >= printData->printedLns-1) {

        if (printData->printedLns > 1)
            wprintw(topWin, "\n");

        println_from_buff(topWin, bottomWin, -1, printData->printedLns);
        
        if (printData->printedLns > AVAILABLE_LNS) 
            printData->lnsUpshifted++;
    }

}


void println_from_buff(WINDOW *topWin, WINDOW *bottomWin, int y, int line) {

    int lastx, lasty;

    get_cursor(bottomWin, lasty, lastx);

    if (y != -1) {
        wmove(topWin, y, 0);
    }

    for (int i = 0; lineBuffer[line-1][i].chars[0] != L'\0'; i++) {
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

void validate_input(WINDOW * topWin, WINDOW *bottomWin, PrintData *printData, char *input, Command *cmdLookup, int numCommands) {

    int argCount = 0, unknownCmd = 0, lastx, lasty;
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
                    display_commands(topWin, bottomWin, printData, cmdLookup, numCommands);
                else if (argCount == 2 && (argument = get_command(allTokens[1], cmdLookup, numCommands, 2)) && argument->commandEnum != HELP){
                    display_usage(topWin, bottomWin, printData, *argument);
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
        get_cursor(topWin, lasty, lastx);
        mvwprintw(topWin, AVAILABLE_LNS, 0, "Unknown command: %s", input);
        restore_cursor(topWin, lasty, lastx);
        wrefresh(topWin);

    }
}

Command * get_command(char *input, Command *cmdLookup, int numCommands, int lookupType) {

    /* skip inital '/' if a command is argument to /help 
     * (eg. /help join instead of /help /join) 
     */
    int skip = lookupType == 1 ? 0 : 1;     

    for (int i = 0; i < numCommands; i++) {
        if (strcmp(input, &cmdLookup[i].name[skip]) == 0)
            return &cmdLookup[i];
    }

    return NULL;
}

void display_commands(WINDOW *topWin, WINDOW *bottomWin, PrintData *printData, Command *cmdLookup, int numCommands) {

    print_empty(topWin, bottomWin, printData);
    print_text_ext(topWin, bottomWin, printData, "", " ** ", "Commands:", 0, CYAN, A_STANDOUT);

    for (int i = 1; i < numCommands; i++) 
        print_text_ext(topWin, bottomWin, printData, "", SPACE, &cmdLookup[i].name[1], 0, 0, 0);

    print_text_ext(topWin, bottomWin, printData, "", SPACE, "For info about a command type /help <command name>.", 0, 0, 0);
  
    wrefresh(topWin);
}

void display_usage(WINDOW *topWin, WINDOW *bottomWin, PrintData *printData, Command command) {

    print_empty(topWin, bottomWin, printData);
    print_text_ext(topWin, bottomWin, printData, "", " ** ", &command.name[1], 0, CYAN, A_STANDOUT);

    for (int i = 0; i < USAGE_LNS; i++)
        print_text_ext(topWin, bottomWin, printData, "", SPACE, command.usage[i], 0, 0, 0);

    wrefresh(topWin);
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
    fprintf(stderr, "%s (code %s)\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}


