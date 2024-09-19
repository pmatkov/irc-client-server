#ifdef TEST
#include "priv_command.h"
#else
#include "command.h"
#endif

#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_TOKEN_LEN 64
#define MAX_CHARS 512

#ifndef TEST

struct Command {
    CommandType commandType;
    CommandHandler commandHandler;     
    char *label;
    char *syntax;
    char *description[MAX_TOKENS];
    char *examples[MAX_TOKENS];
};

#endif

static const Command COMMANDS[] = {

    {HELP, CLIENT, "help", NULL, {NULL}, {NULL}},

    {CONNECT, CLIENT, 
        "connect",  
        "connect <address | hostname> [port]", 
        {"connects to the server with the specified address or hostname and optional port",
        "if no port is provided a default port of 50100 will be used", NULL},
        {"/connect 127.0.0.1 49152", NULL}
        },

    {DISCONNECT, CLIENT, 
        "disconnect", 
        "disconnect [msg]",
        {"disconnects from the active server with optional message", NULL},
        {"/disconnect bye", NULL}
        },

    {NICK, COMMON, 
        "nick", 
        "nick <nickname>",
        {"sets users nickname", NULL},
        {"/nick john", NULL}
        },

    {USER, COMMON, 
        "user", 
        "user [username] [hostname] <real name>",
        {"sets users username, hostname and real name",
         "if no username or hostname is provided default values will be used", NULL},
        {"/user john123 defhost \"john doe\"", NULL}
        },

    {JOIN, COMMON, 
        "join", 
        "join <channel>",
        {"joins the specified channel or creates a new one if it doesn't exist", NULL},
        {"/join #general", NULL}
        },

    {PART, COMMON, 
        "part", 
        "part <channel> [msg]",
        {"leaves the channel with optional message", NULL},
        {"/part #general bye", NULL}
    },

    {PRIVMSG, COMMON, 
        "msg", 
        "msg <channel | user> <message>",
        {"sends message to the channel", NULL},
        {"/msg #programming what is a double pointer?", 
        "/msg john bye", NULL}
        },

    {QUIT, COMMON, 
        "quit", 
        "quit [msg]",
        {"terminates the application with optional message", NULL}, 
        {"/quit done for today!", NULL}
    },

    {UNKNOWN_COMMAND_TYPE, UNKNOWN_COMMAND_HANDLER,
        "unknown", NULL, {NULL}, {NULL}},
};

_Static_assert(sizeof(COMMANDS) / sizeof(COMMANDS[0]) == COMMAND_TYPE_COUNT, "Array size mismatch");

CommandTokens * create_command_tokens(void) {

    CommandTokens *cmdTokens = (CommandTokens *) malloc(sizeof(CommandTokens));
    if (cmdTokens == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    memset(cmdTokens->input, '\0', MAX_CHARS + 1);

    cmdTokens->command = NULL;

    for (int i = 0; i < MAX_TOKENS; i++) {
        cmdTokens->args[i] = NULL;
    }

    cmdTokens->argCount = 0;

    return cmdTokens;
}

void delete_command_tokens(CommandTokens *cmdTokens) {

    free(cmdTokens);
}

const char * command_type_to_string(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMANDS[commandType].label;
    };

    return NULL;
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

// command strings start with '/'
int has_command_prefix(const char *string) {

    if (string == NULL || !strlen(string)) {
        FAILED(NULL, ARG_ERROR);
    }
    return string[0] == '/' ? 1 : 0;
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

void get_command_description(const Command *command, char **descriptionArray, int size) {

    if (command == NULL || descriptionArray == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0;
    while (command->description[i] != NULL) {
        
        descriptionArray[i] = command->description[i];
        i++;
    }
}

void get_command_examples(const Command *command, char **examplesArray, int size) {

    if (command == NULL || examplesArray == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0;
    while (command->examples[i] != NULL) {
        examplesArray[i] = command->examples[i];
        i++;
    }
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
