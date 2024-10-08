/* --INTERNAL HEADER--
   used for unit testing */

#ifndef COMMAND_H
#define COMMAND_H

#include "string_utils.h"

#define MAX_TOKENS 5

typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    NICK,
    USER,
    JOIN,
    PART,
    PRIVMSG,
    QUIT, 
    UNKNOWN_COMMAND_TYPE,
    COMMAND_TYPE_COUNT
} CommandType;

typedef enum {
    CLIENT_COMMAND,
    SERVER_COMMAND,
    COMMON_COMMAND,
    UNKNOWN_COMMAND_USER,
    COMMAND_USER_COUNT
} CommandUser;

typedef struct {
    const char *prefix[MAX_TOKENS];
    const char *body[MAX_TOKENS];
    const char *suffix[MAX_TOKENS];
    int useLeadChar;
} MessageTokens;

typedef struct {
    char input[MAX_CHARS + 1];
    const char *command;
    const char *args[MAX_TOKENS];
    int argCount;
} CommandTokens;

typedef struct {
    CommandType commandType;
    CommandUser commandUser;     
    const char *label;
    const char *syntax;
    const char * const description[MAX_TOKENS];
    const char * const examples[MAX_TOKENS];
} Command;

CommandTokens * create_command_tokens(void);
void delete_command_tokens(CommandTokens *cmdTokens);

void reset_cmd_tokens(CommandTokens *cmdTokens);

void create_message(char *buffer, int size, MessageTokens *messageTokens);

const char * command_type_to_string(CommandType commandType);
CommandType string_to_command_type(const char *string);

int is_valid_command(CommandType commandType);

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