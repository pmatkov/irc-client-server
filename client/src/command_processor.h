#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "scrollback.h"
#include "settings.h"
#include "session.h"

typedef enum {
    HELP,
    CONNECT,
    DISCONNECT,
    NICK,
    USER,
    JOIN,
    PART,
    MSG,
    QUIT, 
    UNKNOWN_COMMAND_TYPE,
    COMMAND_TYPE_COUNT
} CommandType;

typedef enum {
    CLIENT,
    SERVER,
    COMMON,
    COMMAND_OBJECT_COUNT
} CommandObject;

typedef void (*CommandFunction)(Scrollback *, Settings *, Session *, char **, int);
typedef struct Command Command;

CommandFunction get_command_function(CommandType commandType);
const char * get_command_label(CommandType commandType);
int is_valid_command(CommandType commandType);
int has_command_prefix(const char *string);
CommandType string_to_command_type(const char *string);

#endif