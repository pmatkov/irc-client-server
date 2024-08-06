#include "command_parser.h"
#include "display.h"
#include "tcpclient.h"

#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 256
#define MAX_ARGS 3

STATIC int split_input_string(char **cmdTokens, char *input);
STATIC void concat_tokens(char *buffer, int buffSize, char **cmdTokens, int tkCount);
STATIC int has_command_prefix(const char *cmdString);
STATIC Command convert_token_to_cmd(const char *token, int firstToken);

typedef int (*CommandFunction)(char **, int);

int cmd_help(char **cmdTokens, int size);
int cmd_connect(char **cmdTokens, int size);
int cmd_disconnect(char **cmdTokens, int size);
int cmd_join(char **cmdTokens, int size);
int cmd_msg(char **cmdTokens, int size);
int cmd_quit(char **cmdTokens, int size);
int cmd_unknown(char **cmdTokens, int size);

static CommandFunction commandFunctions[CMD_COUNT] = {
    cmd_help,
    cmd_connect,
    cmd_disconnect,
    cmd_join,
    cmd_msg,
    cmd_quit,
    cmd_unknown
};

static const CommandInfo cmdInfo[] = {

    {HELP, "help", ""},

    {CONNECT, "connect", 
        "Syntax:        connect <adddress> [<port>]\n \
        Description:   connects to the server at the specified address and port.\n \
        Example:       /connect 127.0.0.1 49152. Allowed ports: > 49151; default: 50100"},

    {DISCONNECT, "disconnect", 
        "Syntax:        disconnect [<msg>]\n \
        Description:   disconnects from the server with optional message.\n \
        Example:       /disconnect Done for today!"},

    {JOIN, "join", 
        "Syntax:        join <channel>\n \
        Description:   joins the specified channel.\n \
        Example:       /join #general"},

    {MSG, "msg", 
        "Syntax:        msg <channel> <message>\n \
        Description:   sends message to a channel.\n \
        Example:       /msg #general I like pointers."},

    {QUIT, "quit", 
        "Syntax:        quit\n \
        Description:   terminates the application\n \
        Example:       /quit"},

    {UNKNOWN, "unknown", ""},
};


int cmd_help(char **cmdTokens, int size) {

    if (size == 1) {
        print_commands(cmdInfo);
    } 
    else if (size == 2) {
        Command cmd = convert_token_to_cmd(cmdTokens[1], 0);

        if (cmd != UNKNOWN) {
            print_usage(cmdInfo[cmd]);
        }
        else {
            cmd_unknown(cmdTokens, size);
        }
    }
    else {
        cmd_unknown(cmdTokens, size);
    }

    return 1;
}

int cmd_connect(char** cmdTokens, int size) {

    int connectionAttempt = create_connection(cmdTokens[1], cmdTokens[2]);

    if (connectionAttempt == 0) {
        print_info("Connected to %s: %s", cmdTokens[1], cmdTokens[2]);
    }
    else if (connectionAttempt == -1) {
        print_info("Unable to connect to %s: %s", cmdTokens[1], cmdTokens[2]);
    }
    else if (connectionAttempt == -2) {
        print_info("Invalid address: '%s'", cmdTokens[1]);
    } 
    else if (connectionAttempt == -3) {
        print_info("Invalid port: '%s'", cmdTokens[2]);
    }
    return 1;
}

int cmd_disconnect(char** cmdTokens, int size) {
    return 1;
}

int cmd_join(char** cmdTokens, int size) {
    return 1;
}

int cmd_msg(char** cmdTokens, int size) {
    return 1;
}

int cmd_quit(char** cmdTokens, int size) {
    
    return 0;
}

int cmd_unknown(char** cmdTokens, int size) {

    char buffer[MAX_CHARS+1] = {'\0'};
    concat_tokens(buffer, MAX_CHARS, cmdTokens, size);

    print_info("Unknown command: %s", &buffer[1]);

    return 1;
}

_Static_assert(sizeof(cmdInfo) / sizeof(cmdInfo[0]) == CMD_COUNT, "cmdInfo array size and CMD_COUNT don't match");


// splits command string to tokens
STATIC int split_input_string(char **cmdTokens, char *input) {

    char *token, *savePtr;
    int tkCount = 0;

    token = strtok_r(input, " ", &savePtr);

    while (token && tkCount < MAX_ARGS) {

        cmdTokens[tkCount++] = token;
        token = strtok_r(NULL, " ", &savePtr);
    }
    return tkCount;
}

// concatenate command tokens
STATIC void concat_tokens(char *buffer, int buffSize, char **cmdTokens, int tkCount) {

    int i = 0;
    while (i < tkCount) {
        strncat(buffer, cmdTokens[i++], buffSize - strlen(buffer) - 1);
        strncat(buffer, " ", buffSize - strlen(buffer) - 1);
    }
    buffer[strlen(buffer)-1] = '\0';
}

// command string should start with '/'
int has_command_prefix(const char *cmdString) {

    return cmdString[0] == '/' ? 1 : 0;
}

// convert token to enum
STATIC Command convert_token_to_cmd(const char *token, int firstToken) {

    int startPos = firstToken ? 1 : 0;

    for (int i = 0; i < CMD_COUNT; i++) {

        if (strcmp(&token[startPos], cmdInfo[i].label) == 0) {
            return cmdInfo[i].command;
        }
    }
    return UNKNOWN;
}

// parse command line input
int parse_input(Scrollback *scrollback, LineEditor *lnEditor) {

    if (!lnEditor->charCount) {
        return;
    }

    int terminated = 0;

    // split input to tokens and call corresponding function

    if (has_command_prefix(lnEditor->buffer)) {

        lnEditor->buffer[lnEditor->charCount] = '\0';

        char *inputCopy = strdup(lnEditor->buffer);
        char *tokens[MAX_ARGS] = {NULL};

        int tokenCount = split_input_string(tokens, inputCopy);

        Command cmd = convert_token_to_cmd(tokens[0], 1);

        int terminated = commandFunctions[cmd](tokens, tokenCount);

        free(inputCopy);

    }

    delete_line_from_pos(lnEditor->window, PROMPT_SIZE);

    lnEditor->charCount = 0;
    lnEditor->cursor = PROMPT_SIZE;

    wrefresh(lnEditor->window);
    return terminated;

}
