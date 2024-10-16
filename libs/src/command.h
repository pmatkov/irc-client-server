#ifndef COMMAND_H
#define COMMAND_H

#include "string_utils.h"

#define MAX_TOKENS 5

/* commandType represents a list of commands
    supported by the client and the server */
typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    NICK,
    USER,
    JOIN,
    PART,
    PRIVMSG,
    SERVER_ADDRESS,
    SERVER_PORT,
    QUIT, 
    UNKNOWN_COMMAND_TYPE,
    COMMAND_TYPE_COUNT
} CommandType;

/* commandUser represents the app which may
    execute the command, either the client, 
    the server or both */
typedef enum {
    CLIENT_COMMAND,
    SERVER_COMMAND,
    COMMON_COMMAND,
    UNKNOWN_COMMAND_USER,
    COMMAND_USER_COUNT
} CommandUser;

/* the client receives commands from the terminal 
    and the server from the network. each command
    string (either command line input or network 
    message) is parsed into tokens. these tokens are 
    then divided to an actual command and command 
    parameters */
typedef struct {
    char input[MAX_CHARS + 1];
    const char *command;
    const char *args[MAX_TOKENS];
    int argCount;
} CommandTokens;

/* command is a container for all information related
    to commands. commands parsed by the client contain
    a label, a syntax, a description and examples */
typedef struct Command Command;

CommandTokens * create_command_tokens(void);
void delete_command_tokens(CommandTokens *cmdTokens);

/* reset command tokens to default values */
void reset_cmd_tokens(CommandTokens *cmdTokens);

const char * command_type_to_string(CommandType commandType);
CommandType string_to_command_type(const char *string);

int is_valid_command(CommandType commandType);

/* a valid command line command should start with 
    '/' prefix */
int has_command_prefix(const char *string);

const Command * get_commands(void);
const Command * get_command(CommandType commandType);
int get_command_size(void);

const char * get_command_label(const Command *command);
const char * get_command_syntax(const Command *command);
const char ** get_command_description(const Command *command);
const char ** get_command_examples(const Command *command);

const char * get_cmd_from_cmd_tokens(CommandTokens *cmdTokens);
const char * get_arg_from_cmd_tokens(CommandTokens *cmdTokens, int index);
int get_arg_count_from_cmd_tokens(CommandTokens *cmdTokens);

#endif