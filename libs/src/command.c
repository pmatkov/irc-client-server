#ifdef TEST
#include "priv_command.h"
#else
#include "command.h"
#include "common.h"
#endif

#include "enum_utils.h"
#include "error_control.h"
#include "logger.h"

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

struct CommandTokens {
    char input[MAX_CHARS + 1];
    const char *command;
    const char *args[MAX_TOKENS];
    int argCount;
};

/*  a command is identified by its type, its user and
    its label. additionally, client's commands include
    syntax, description and examples */
struct CommandInfo {
    CommandType cmdType;
    CommandUser commandUser;
    int minArgs;
    int maxArgs;  
    const char *label;
    const char *alias;
    const char *syntax;
    const char * const description[MAX_TOKENS];
    const char * const examples[MAX_TOKENS];
};

#endif

/* a list of commands with their assigned properties.
    the last item in the array is NULL to enable 
    convenient iteration over array elements */
static const CommandInfo *COMMAND_INFOS[] = {

    &(CommandInfo){
        HELP, CLIENT_COMMAND, 
        0, 1,
        "help", NULL,
        NULL, {NULL}, {NULL}},

    &(CommandInfo){
        CONNECT, CLIENT_COMMAND, 
        0, 0, 
        "connect", NULL,
        "connect [IP address | hostname] [port]", 
        {"connects to the server at the optional IP address (hostname) and port",
        "if no IP address is provided <127.0.0.1> will be used", 
        "if no port is provided <50100> will be used", NULL},
        {"/connect 127.0.0.1 49152", NULL}
        },

    &(CommandInfo){
        DISCONNECT, CLIENT_COMMAND,
        0, 1,
        "disconnect", NULL,
        "disconnect [msg]",
        {"disconnects from the active server with an optional message", NULL},
        {"/disconnect bye", NULL}
        },

    &(CommandInfo){
        NICK, COMMON_COMMAND,
        1, 1, 
        "nick", NULL,
        "nick <nickname>",
        {"sets user nickname", NULL},
        {"/nick j007", NULL}
        },

    &(CommandInfo){
        USER, COMMON_COMMAND, 
        1, 3,
        "user", NULL,
        "user [username] [hostname] <real name>",
        {"sets user username, hostname and real name",
         "if no username or hostname is provided default values will be used", NULL},
        {"/user jjones defhost \"john jones\"", NULL}
        },

    &(CommandInfo){
        JOIN, COMMON_COMMAND, 
        1, 1,
        "join", NULL,
        "join <channel>",
        {"joins the specified channel or creates a new one if it doesn't exist", NULL},
        {"/join #general", NULL}
        },

    &(CommandInfo){
        PART, COMMON_COMMAND, 
        1, 2,
        "part", NULL, 
        "part <channel> [msg]",
        {"leaves the channel with optional message", NULL},
        {"/part #general bye", NULL}
    },

    &(CommandInfo){
        PRIVMSG, COMMON_COMMAND, 
        2, 2,
        "msg", "privmsg",
        "msg <channel | user> <message>",
        {"sends message to the channel", NULL},
        {"/msg #programming what is a double pointer?", 
        "/msg john bye", NULL}
        },
        
    &(CommandInfo){
        ADDRESS, CLIENT_COMMAND, 
        1, 1,
        "address", NULL,
        "address <address>",
        {"sets a default server address", NULL},
        {"/address 45.56.126.124", 
        "/address chat.freenode.net", NULL}
        },

    &(CommandInfo){
        PORT, CLIENT_COMMAND, 
        1, 1,
        "port", NULL,
        "port <port>",
        {"sets a default server port", NULL},
        {"/port 50100", NULL}
        },

    &(CommandInfo){
        WHOIS, COMMON_COMMAND, 
        1, 1,
        "whois", NULL,
        "whois <user>",
        {"displays detailed information about a specific user", NULL}, 
        {"/whois john", NULL}
    },

    &(CommandInfo){
        QUIT, COMMON_COMMAND, 
        0, 1,
        "quit", NULL,
        "quit [msg]",
        {"terminates the application with optional message", NULL}, 
        {"/quit done for today!", NULL}
    },

    &(CommandInfo){
        UNKNOWN_COMMAND_TYPE, UNKNOWN_COMMAND_USER,
        0, 0,
        "unknown", NULL, 
        NULL, {NULL}, {NULL}},

    NULL
};

ASSERT_ARRAY_SIZE(COMMAND_INFOS, COMMAND_TYPE_COUNT + 1)

CommandTokens * create_command_tokens(int count) {

    CommandTokens *cmdTokens = (CommandTokens *) malloc(count * sizeof(CommandTokens));
    if (cmdTokens == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < count; i++) {
        reset_command_tokens(&cmdTokens[i]);
    }

    return cmdTokens;
}

void delete_command_tokens(CommandTokens *cmdTokens) {

    free(cmdTokens);
}

void reset_command_tokens(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    memset(cmdTokens->input, '\0', sizeof(cmdTokens->input));

    cmdTokens->command = NULL;

    for (int i = 0; i < MAX_TOKENS; i++) {
        cmdTokens->args[i] = NULL;
    }

    cmdTokens->argCount = 0;
}


CommandType string_to_command_type(const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    CommandType cmdType = UNKNOWN_COMMAND_TYPE;

    int startIndex = has_command_prefix(string) ? 1 : 0;
    char lcaseString[MAX_CHARS + 1] = {'\0'};

    str_to_lower(lcaseString, ARRAY_SIZE(lcaseString), &string[startIndex]);

    for (int i = 0; i < COMMAND_TYPE_COUNT; i++) {

        if (strncmp(COMMAND_INFOS[i]->label, lcaseString, MAX_TOKEN_LEN) == 0 || \
            (COMMAND_INFOS[i]->alias != NULL && strncmp(COMMAND_INFOS[i]->alias, lcaseString, MAX_TOKEN_LEN) == 0)) {
            cmdType = COMMAND_INFOS[i]->cmdType;
            break;
        }
    }
    return cmdType;
}

bool has_command_prefix(const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return strlen(string) && string[0] == '/';
}

const CommandInfo * get_cmd_info(CommandType cmdType) {

    const CommandInfo *commandInfo = COMMAND_INFOS[UNKNOWN_COMMAND_TYPE];
    
    if (is_valid_enum_type(cmdType, COMMAND_TYPE_COUNT)) {
        commandInfo = COMMAND_INFOS[cmdType];
    }

    return commandInfo;
}

const CommandInfo ** get_cmd_infos(void) {

    return COMMAND_INFOS;
}

int get_cmd_info_min_args(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return commandInfo->minArgs;
}

int get_cmd_info_max_args(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return commandInfo->maxArgs;
}

const char * get_cmd_info_label(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return commandInfo->label;
}

const char * get_cmd_info_syntax(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return commandInfo->syntax;
}

const char ** get_cmd_info_description(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (const char **) commandInfo->description;
}

const char ** get_cmd_info_examples(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (const char **) commandInfo->examples;
}

char * get_command_input(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return cmdTokens->input;
}

const char * get_command(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return cmdTokens->command;
}

void set_command(CommandTokens *cmdTokens, const char *command) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    cmdTokens->command = command; 
}

const char * get_command_argument(CommandTokens *cmdTokens, int index) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *arg = NULL;

    if (index >= 0 && index < MAX_TOKENS) {
        arg = cmdTokens->args[index];
    }

    return arg;
}

const char ** get_command_arguments(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (const char **) cmdTokens->args;
}

void set_command_argument(CommandTokens *cmdTokens, const char *arg, int index) {

    if (cmdTokens == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (index >= 0 && index < MAX_TOKENS) {
        cmdTokens->args[index] = arg;
    }
}

int get_command_argument_count(CommandTokens *cmdTokens) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return cmdTokens->argCount;
}

void set_command_argument_count(CommandTokens *cmdTokens, int argCount) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    cmdTokens->argCount = argCount;
}

