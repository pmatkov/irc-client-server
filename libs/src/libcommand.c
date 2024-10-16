#ifdef TEST
#include "priv_command.h"
#else
#include "command.h"
#endif

#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_TOKEN_LEN 64

#ifndef TEST

/*  the label, syntax, description and 
    examples are used only by the client
    to display this information to the user */
struct Command {
    CommandType commandType;
    CommandUser commandUser;     
    const char *label;
    const char *syntax;
    const char * const description[MAX_TOKENS];
    const char * const examples[MAX_TOKENS];
};

#endif

/* a collection of all available commands along
    with their assigned values. each array of 
    pointers ends with a NULL pointer item which
    enables convenient identification of the last 
    item */
static const Command COMMANDS[] = {

    {HELP, CLIENT_COMMAND, "help", NULL, {NULL}, {NULL}},

    {CONNECT, CLIENT_COMMAND, 
        "connect",  
        "connect <address | hostname> [port]", 
        {"connects to the server with the specified address or hostname and optional port",
        "if no port is provided a default port of 50100 will be used", NULL},
        {"/connect 127.0.0.1 49152", NULL}
        },

    {DISCONNECT, CLIENT_COMMAND, 
        "disconnect", 
        "disconnect [msg]",
        {"disconnects from the active server with optional message", NULL},
        {"/disconnect bye", NULL}
        },

    {NICK, COMMON_COMMAND, 
        "nick", 
        "nick <nickname>",
        {"sets users nickname", NULL},
        {"/nick john", NULL}
        },

    {USER, COMMON_COMMAND, 
        "user", 
        "user [username] [hostname] <real name>",
        {"sets users username, hostname and real name",
         "if no username or hostname is provided default values will be used", NULL},
        {"/user john123 defhost \"john doe\"", NULL}
        },

    {JOIN, COMMON_COMMAND, 
        "join", 
        "join <channel>",
        {"joins the specified channel or creates a new one if it doesn't exist", NULL},
        {"/join #general", NULL}
        },

    {PART, COMMON_COMMAND, 
        "part", 
        "part <channel> [msg]",
        {"leaves the channel with optional message", NULL},
        {"/part #general bye", NULL}
    },

    {PRIVMSG, COMMON_COMMAND, 
        "msg", 
        "msg <channel | user> <message>",
        {"sends message to the channel", NULL},
        {"/msg #programming what is a double pointer?", 
        "/msg john bye", NULL}
        },
        
    {SERVER_ADDRESS, CLIENT_COMMAND, 
        "address", 
        "address <address>",
        {"sets a default server address", NULL},
        {"/address 45.56.126.124", 
        "/address chat.freenode.net", NULL}
        },

    {SERVER_PORT, CLIENT_COMMAND, 
        "port", 
        "port <port>",
        {"sets a default server port", NULL},
        {"/port 50100", NULL}
        },

    {QUIT, COMMON_COMMAND, 
        "quit", 
        "quit [msg]",
        {"terminates the application with optional message", NULL}, 
        {"/quit done for today!", NULL}
    },

    {UNKNOWN_COMMAND_TYPE, UNKNOWN_COMMAND_USER,
        "unknown", NULL, {NULL}, {NULL}},
};

static_assert(ARR_SIZE(COMMANDS) == COMMAND_TYPE_COUNT, "Array size mismatch");

CommandTokens * create_command_tokens(void) {

    CommandTokens *cmdTokens = (CommandTokens *) malloc(sizeof(CommandTokens));
    if (cmdTokens == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    reset_cmd_tokens(cmdTokens);    

    return cmdTokens;
}

void delete_command_tokens(CommandTokens *cmdTokens) {

    free(cmdTokens);
}

void reset_cmd_tokens(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    memset(cmdTokens->input, '\0', sizeof(cmdTokens->input));

    cmdTokens->command = NULL;

    for (int i = 0; i < MAX_TOKENS; i++) {
        cmdTokens->args[i] = NULL;
    }

    cmdTokens->argCount = 0;
}

const char * command_type_to_string(CommandType commandType) {

    const char *string = NULL;

    if (is_valid_command(commandType)) {
        string = COMMANDS[commandType].label;
    };

    return string;
}

CommandType string_to_command_type(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    CommandType commandType = UNKNOWN_COMMAND_TYPE;

    int startIndex = has_command_prefix(string) ? 1 : 0;

    for (int i = 0; i < COMMAND_TYPE_COUNT; i++) {

        if (strncmp(COMMANDS[i].label, &string[startIndex], MAX_TOKEN_LEN) == 0) {
            commandType = COMMANDS[i].commandType;
        }
    }
    return commandType;
}

int is_valid_command(CommandType commandType) {

    return commandType >= 0 && commandType < COMMAND_TYPE_COUNT;
}

int has_command_prefix(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int prefix = 0;

    if (strlen(string) && string[0] == '/' ) {

        prefix = 1;
    }

    return prefix;
}

const Command * get_commands(void) {

    return COMMANDS;
}

int get_command_size(void) {

    return sizeof(COMMANDS[0]);
}

const Command * get_command(CommandType commandType) {

    const Command *command = &COMMANDS[UNKNOWN_COMMAND_TYPE];
    
    if (is_valid_command(commandType)) {
        command = &COMMANDS[commandType];
    }

    return command;
}

const char * get_command_label(const Command *command) {

    if (command == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return command->label;
}

const char * get_command_syntax(const Command *command) {

    if (command == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return command->syntax;
}

const char ** get_command_description(const Command *command) {

    if (command == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return (const char **) command->description;
}

const char ** get_command_examples(const Command *command) {

    if (command == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return (const char **) command->examples;
}

const char * get_cmd_from_cmd_tokens(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return cmdTokens->command;
}

const char * get_arg_from_cmd_tokens(CommandTokens *cmdTokens, int index) {

    if (cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char *arg = NULL;

    if (index >= 0 && index < MAX_TOKENS) {
        arg = cmdTokens->args[index];
    }

    return arg;
}

int get_arg_count_from_cmd_tokens(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return cmdTokens->argCount;
}
