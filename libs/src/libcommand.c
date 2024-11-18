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

struct CommandTokens{
    char input[MAX_CHARS + 1];
    const char *command;
    const char *args[MAX_TOKENS];
    int argCount;
};

/*  every commandInfo is identified by its type, its user and
    its label. additionally, client commands include
    syntax, description and examples */
struct CommandInfo {
    CommandType commandType;
    CommandUser commandUser;     
    const char *label;
    const char *syntax;
    const char * const description[MAX_TOKENS];
    const char * const examples[MAX_TOKENS];
};

#endif

/* a list of all commands with their assigned 
    properties. the last item in the designated
    initializer array is NULL to enable convenient
    iteration over array elements */
static const CommandInfo COMMAND_INFOS[] = {

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
        
    {ADDRESS, CLIENT_COMMAND, 
        "address", 
        "address <address>",
        {"sets a default server address", NULL},
        {"/address 45.56.126.124", 
        "/address chat.freenode.net", NULL}
        },

    {PORT, CLIENT_COMMAND, 
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

static_assert(ARR_SIZE(COMMAND_INFOS) == COMMAND_TYPE_COUNT, "Array size mismatch");

CommandTokens * create_command_tokens(int count) {

    CommandTokens *cmdTokens = (CommandTokens *) malloc(count * sizeof(CommandTokens));
    if (cmdTokens == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
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

const char * command_type_to_string(CommandType commandType) {

    const char *string = NULL;

    if (is_valid_command(commandType)) {
        string = COMMAND_INFOS[commandType].label;
    };

    return string;
}

CommandType string_to_command_type(const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    CommandType commandType = UNKNOWN_COMMAND_TYPE;

    int startIndex = has_command_prefix(string) ? 1 : 0;

    for (int i = 0; i < COMMAND_TYPE_COUNT; i++) {

        if (strncmp(COMMAND_INFOS[i].label, &string[startIndex], MAX_TOKEN_LEN) == 0) {
            commandType = COMMAND_INFOS[i].commandType;
        }
    }
    return commandType;
}

int is_valid_command(CommandType commandType) {

    return commandType >= 0 && commandType < COMMAND_TYPE_COUNT;
}

int has_command_prefix(const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int prefix = 0;

    if (strlen(string) && string[0] == '/' ) {

        prefix = 1;
    }

    return prefix;
}

const CommandInfo * get_command_info(CommandType commandType) {

    const CommandInfo *commandInfo = &COMMAND_INFOS[UNKNOWN_COMMAND_TYPE];
    
    if (is_valid_command(commandType)) {
        commandInfo = &COMMAND_INFOS[commandType];
    }

    return commandInfo;
}

const CommandInfo * get_command_infos(void) {

    return COMMAND_INFOS;
}

int get_command_info_size(void) {

    return sizeof(COMMAND_INFOS[0]);
}

const char * get_command_info_label(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return commandInfo->label;
}

const char * get_command_info_syntax(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return commandInfo->syntax;
}

const char ** get_command_info_description(const CommandInfo *commandInfo) {

    if (commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (const char **) commandInfo->description;
}

const char ** get_command_info_examples(const CommandInfo *commandInfo) {

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
