#ifndef COMMAND_H
#define COMMAND_H

#define MAX_TOKENS 5
#define MAX_CHARS 512

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
    CLIENT,
    SERVER,
    COMMON,
    UNKNOWN_COMMAND_HANDLER,
    COMMAND_HANDLER_COUNT
} CommandHandler;

typedef struct {
    char input[MAX_CHARS + 1];
    const char *command;
    const char *args[MAX_TOKENS];
    int argCount;
} CommandTokens;

typedef struct Command Command;

CommandTokens * create_command_tokens(void);
void delete_command_tokens(CommandTokens *cmdTokens);

const char * command_type_to_string(CommandType commandType);
CommandType string_to_command_type(const char *string);

int is_valid_command(CommandType commandType);

const Command * get_commands(void);
const Command * get_command(CommandType commandType);
int get_command_size(void);

const char * get_command_label(const Command *command);
const char * get_command_syntax(const Command *command);
void get_command_description(const Command *command, char **descriptionArray, int size);
void get_command_examples(const Command *command, char **examplesArray, int size);

const char * get_cmd_from_cmd_tokens(CommandTokens *cmdTokens);
const char * get_arg_from_cmd_tokens(CommandTokens *cmdTokens, int index);
int get_arg_count_from_cmd_tokens(CommandTokens *cmdTokens);

int has_command_prefix(const char *string);

#endif