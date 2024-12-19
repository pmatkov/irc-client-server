#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

#define MAX_TOKENS 5

/* represents a list of commands */
typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    NICK,
    USER,
    JOIN,
    PART,
    PRIVMSG,
    ADDRESS,
    PORT,
    WHOIS,
    QUIT,
    UNKNOWN_COMMAND_TYPE,
    COMMAND_TYPE_COUNT
} CommandType;

/* represents an app that may execute the command */
typedef enum {
    CLIENT_COMMAND,
    SERVER_COMMAND,
    COMMON_COMMAND,
    UNKNOWN_COMMAND_USER,
    COMMAND_USER_COUNT
} CommandUser;

/* commands are parsed into a series of tokens. the 
    command string is stored in the command token 
    while the arguments are stored in the args array */
typedef struct CommandTokens CommandTokens;

/* contains command info like syntax, description and 
    examples */
typedef struct CommandInfo CommandInfo;

CommandTokens * create_command_tokens(int count);
void delete_command_tokens(CommandTokens *cmdTokens);

/* reset command tokens to default values */
void reset_command_tokens(CommandTokens *cmdTokens);

CommandType string_to_command_type(const char *string);

bool has_command_prefix(const char *string);

const CommandInfo * get_cmd_info(CommandType cmdType);
const CommandInfo ** get_cmd_infos(void);

const char * get_cmd_info_label(const CommandInfo *commandInfo);
const char * get_cmd_info_syntax(const CommandInfo *commandInfo);
const char ** get_cmd_info_description(const CommandInfo *commandInfo);
const char ** get_cmd_info_examples(const CommandInfo *commandInfo);

char * get_command_input(CommandTokens *cmdTokens);
const char * get_command(CommandTokens *cmdTokens);
void set_command(CommandTokens *cmdTokens, const char *command);
const char ** get_command_arguments(CommandTokens *cmdTokens);
const char * get_command_argument(CommandTokens *cmdTokens, int index);
void set_command_argument(CommandTokens *cmdTokens, const char *arg, int index);
int get_command_argument_count(CommandTokens *cmdTokens);
void set_command_argument_count(CommandTokens *cmdTokens, int argCount);

#endif