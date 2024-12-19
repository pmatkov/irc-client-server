/* --INTERNAL HEADER--
   used for testing */
#ifndef COMMAND_H
#define COMMAND_H

#include "common.h"

#include <stdbool.h>

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
    WHOIS,
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
    CommandType cmdType;
    CommandUser commandUser;
    int minArgs;
    int maxArgs;       
    const char *label;
    const char *syntax;
    const char * const description[MAX_TOKENS];
    const char * const examples[MAX_TOKENS];
} CommandInfo;

CommandTokens * create_command_tokens(int count);
void delete_command_tokens(CommandTokens *cmdTokens);

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
const char * get_command_argument(CommandTokens *cmdTokens, int index);
const char ** get_command_arguments(CommandTokens *cmdTokens);
void set_command_argument(CommandTokens *cmdTokens, const char *arg, int index);
int get_command_argument_count(CommandTokens *cmdTokens);
void set_command_argument_count(CommandTokens *cmdTokens, int argCount);

#endif