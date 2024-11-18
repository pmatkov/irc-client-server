/* --INTERNAL HEADER--
   used for testing */

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
    ADDRESS,
    PORT,
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
    char input[MAX_CHARS + 1];
    const char *command;
    const char * args[MAX_TOKENS];
    int argCount;
} CommandTokens;

typedef struct {
    CommandType commandType;
    CommandUser commandUser;     
    const char *label;
    const char *syntax;
    const char * const description[MAX_TOKENS];
    const char * const examples[MAX_TOKENS];
} CommandInfo;

CommandTokens * create_command_tokens(int count);
void delete_command_tokens(CommandTokens *cmdTokens);

void reset_command_tokens(CommandTokens *cmdTokens);

const char * command_type_to_string(CommandType commandType);
CommandType string_to_command_type(const char *string);

int is_valid_command(CommandType commandType);

int has_command_prefix(const char *string);

const CommandInfo * get_command_infos(void);
const CommandInfo * get_command_info(CommandType commandType);
int get_command_info_size(void);

const char * get_command_info_label(const CommandInfo *commandInfo);
const char * get_command_info_syntax(const CommandInfo *commandInfo);
const char ** get_command_info_description(const CommandInfo *commandInfo);
const char ** get_command_info_examples(const CommandInfo *commandInfo);

char * get_command_input(CommandTokens *cmdTokens);
const char * get_command(CommandTokens *cmdTokens);
void set_command(CommandTokens *cmdTokens, const char *command);
const char * get_command_argument(CommandTokens *cmdTokens, int index);
const char ** get_command_arguments(CommandTokens *cmdTokens);
void set_command_argument(CommandTokens *cmdTokens, const char *arg, int index);
int get_command_argument_count(CommandTokens *cmdTokens);
void set_command_argument_count(CommandTokens *cmdTokens, int argCount);

#endif