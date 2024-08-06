#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "scrollback.h"
#include "input_handler.h"

#include <ncursesw/curses.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    JOIN,
    MSG,
    QUIT, 
    UNKNOWN,
    CMD_COUNT
} Command;

typedef struct {
    Command command;
    const char *label;
    const char *usage;       
} CommandInfo;

#ifdef TEST
STATIC int split_input_string(char **cmdTokens, char *input);
STATIC void concat_tokens(char *buffer, int buffSize, char **cmdTokens, int tkCount);
STATIC int has_command_prefix(const char *cmdString);
STATIC Command convert_token_to_cmd(const char *token, int firstToken);
#endif

int parse_input(Scrollback *scrollback, LineEditor *lnEditor);

#endif