#include "command_processor.h"
#include "display.h"
#include "tcpclient.h"
#include "../../shared/src/parser.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMAND_LEN 32
#define MAX_MSG_LEN 512

struct Command {
    CommandType commandType;
    CommandObject commandObject;     
    const char *label;
    const char *usage;
};

STATIC void cmd_help(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_connect(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_disconnect(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_nick(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_user(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_join(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_part(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_msg(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_quit(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);
STATIC void cmd_unknown(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount);

static const CommandFunction COMMAND_FUNCTIONS[] = {
    cmd_help,
    cmd_connect,
    cmd_disconnect,
    cmd_nick,
    cmd_user,
    cmd_join,
    cmd_part,
    cmd_msg,
    cmd_quit,
    cmd_unknown
};

static const Command COMMANDS[] = {

    {HELP, CLIENT, "help", ""},

    {CONNECT, CLIENT, "connect",  
        "Syntax:         connect <address | hostname> [port]\nDescription:    connects to the server with the specified address or hostname and optional port\n                if no port is provided a default port of 50100 will be used\nExample(s):     /connect 127.0.0.1 49152"
        },

    {DISCONNECT, CLIENT, "disconnect", 
        "Syntax:         disconnect [msg]\n Description:    disconnects from the active server with optional message\nExample(s):     /disconnect bye"
        },

    {NICK, COMMON, "nick", 
        "Syntax:         nick <nickname>\nDescription:    sets users nickname\nExample(s):     /nick john"
        },

    {USER, COMMON, "user", 
        "Syntax:         user [username] [hostname] <real name> \nDescription:    sets users username, hostname and real name\n                if no username or hostname is provided default values will be used\nExample(s):     /user john123 defhost \"john doe\""
        },

    {JOIN, SERVER, "join", 
        "Syntax:         join <channel>\nDescription:    joins the specified channel or creates a new one if it doesn't exist\nExample(s):     /join #general"
        },

    {PART, SERVER, "part", 
        "Syntax:         part [msg]\nDescription:   leaves the channel with optional message\nExample(s):    /part bye"
        },

    {MSG, SERVER, "msg", 
        "Syntax:         msg <channel | user> <message>\nDescription:    sends message to the channel\nExample(s):     /msg #programming what is a double pointer?\n                /msg john bye"
        },

    {QUIT, COMMON, "quit", 
        "Syntax:         quit [msg]\nDescription:    terminates the application with optional message\nExample(s):     /quit done for today!"},

    {UNKNOWN_COMMAND_TYPE, COMMON, "unknown", ""},
};

_Static_assert(sizeof(COMMANDS) / sizeof(COMMANDS[0]) == COMMAND_TYPE_COUNT, "Array size mismatch");
_Static_assert(sizeof(COMMAND_FUNCTIONS) / sizeof(COMMAND_FUNCTIONS[0]) == COMMAND_TYPE_COUNT, "Array size mismatch");

CommandFunction get_command_function(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMAND_FUNCTIONS[commandType];
    }
    return cmd_unknown;
}


const char * get_command_label(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMANDS[commandType].label;
    };

    return NULL;
}

int is_valid_command(CommandType commandType) {

    return commandType >= 0 && commandType < COMMAND_TYPE_COUNT;
}

// command string starts with '/'
int has_command_prefix(const char *string) {

    if (string == NULL || !strlen(string)) {
        FAILED(NULL, ARG_ERROR);
    }
    return string[0] == '/' ? 1 : 0;
}

CommandType string_to_command_type(const char *string) {

    if (string == NULL || !strlen(string)) {
        FAILED(NULL, ARG_ERROR);
    }

    int startPos = has_command_prefix(string) ? 1 : 0;

    for (int i = 0; i < COMMAND_TYPE_COUNT; i++) {

        if (strncmp(COMMANDS[i].label, &string[startPos], MAX_COMMAND_LEN) == 0) {
            return COMMANDS[i].commandType;
        }
    }
    return UNKNOWN_COMMAND_TYPE;
}

STATIC void cmd_help(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {
        display_commands(scrollback);
    } 
    else if (tkCount == 2) {

        CommandType commandType = string_to_command_type(tokens[1]);

        if (commandType != UNKNOWN_COMMAND_TYPE) {
            display_usage(scrollback, commandType, COMMANDS[commandType].usage);
        }
        else {
            cmd_unknown(scrollback, settings, session, tokens, tkCount);
        }
    }
    else {
        cmd_unknown(scrollback, settings, session, tokens, tkCount);
    }
}

// not transform -> send nick + user
STATIC void cmd_connect(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (session_is_connected(session)) {
        display_response(scrollback, "Already connected. Disconnect before connecting.", tokens[1], tokens[2]);
    }

    int status = connect_to_server(tokens[1], tokens[2], session);

    if (status == 0) {
        display_response(scrollback, "Connected to %s: %s.", tokens[1], tokens[2]);
    }
    else if (status == -1) {
        display_response(scrollback, "Unable to connect to %s: %s.", tokens[1], tokens[2]);
    }
    else if (status == -2) {
        display_response(scrollback, "Invalid address: %s.", tokens[1]);
    } 
    else if (status == -3) {
        display_response(scrollback, "Invalid port: %s.", tokens[2]);
    }

    // add NICK message to the message queue
    char *msgTokens1[] = {"NICK", get_property_value(settings, NICKNAME)};  
    char message[MAX_MSG_LEN + 1] = {'\0'};

    concat_tokens(message, MAX_MSG_LEN + 1, msgTokens1, ARR_SIZE(msgTokens1), " ");
    add_message(session_get_inqueue(session), message);

    // add USER message to the message queue
    char *msgTokens2[] = {"USER", get_property_value(settings, USERNAME), get_property_value(settings, HOSTNAME), ":", get_property_value(settings, REALNAME)};  

    memset(message, '\0', MAX_MSG_LEN + 1);
    concat_tokens(message, MAX_MSG_LEN + 1, msgTokens2, ARR_SIZE(msgTokens2), " ");

    add_message(session_get_inqueue(session), message);
}

STATIC void cmd_disconnect(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {

        add_message(session_get_inqueue(session), "DISCONNECT");
    }
    else {
        char *msgTokens[] = {"DISCONNECT", ":", tokens[1], tokens[2]}; 
        char message[MAX_MSG_LEN + 1] = {'\0'};
        
        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");

        add_message(session_get_inqueue(session), message);
    }
}

STATIC void cmd_nick(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {
        display_response(scrollback, "Missing argument <nickname>.");
    }
    else if (tkCount > 2) {
        display_response(scrollback, "Nickname can only be a single word.");
    }
    else if (tkCount == 2 && strlen(tokens[1]) > 9) {
        display_response(scrollback, "Nickname can have up to 9 chars.");
    }
    else {

        set_property(settings, NICKNAME, tokens[1]);

        char *msgTokens[] = {"NICK", ":", tokens[1]}; 
        char message[MAX_MSG_LEN + 1] = {'\0'};
        
        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");

        add_message(session_get_inqueue(session), message);
    }
}

STATIC void cmd_user(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    char message[MAX_MSG_LEN + 1] = {'\0'};

    if (tkCount < 3) {
        display_response(scrollback, "Wrong number of arguments");
    }
    else if (tkCount == 3) {

        set_property(settings, USERNAME, tokens[1]);
        set_property(settings, HOSTNAME, tokens[2]);

        char *msgTokens[] = {"USER", tokens[1], tokens[2]}; 
     
        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");
        add_message(session_get_inqueue(session), message);
    }
    else {

        set_property(settings, USERNAME, tokens[1]);
        set_property(settings, HOSTNAME, tokens[2]);
        set_property(settings, REALNAME, tokens[3]);

        char *msgTokens[] = {"USER", tokens[1], tokens[2], ":", tokens[3]}; 
        
        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");
        add_message(session_get_inqueue(session), message);
    }
}

STATIC void cmd_join(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {
        display_response(scrollback, "Missing argument <channel>.");
    }
    else if (tkCount == 2) {

        char *msgTokens[] = {"JOIN", tokens[1]}; 
        char message[MAX_MSG_LEN + 1] = {'\0'};

        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");

        add_message(session_get_inqueue(session), message);
    }
    else {

        cmd_unknown(scrollback, settings, session, tokens, tkCount);
    }
}

STATIC void cmd_part(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {

       add_message(session_get_inqueue(session), "PART");
    }
    else {

        char *msgTokens[] = {"PART", ":", tokens[1], tokens[2]}; 
        char message[MAX_MSG_LEN + 1] = {'\0'};

        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");

        add_message(session_get_inqueue(session), message);
    }
 
}

STATIC void cmd_msg(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {

        char *msgTokens[] = {"MSG", ":", tokens[1]};
        char message[MAX_MSG_LEN + 1] = {'\0'};

        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");
        add_message(session_get_inqueue(session), message);

    }
    else if (tkCount == 2) {

        display_response(scrollback, "Missing argument <message>.");
    }
    else {

        char *msgTokens[] = {"MSG", tokens[1], ":", tokens[2]};
        char message[MAX_MSG_LEN + 1] = {'\0'};

        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");
        add_message(session_get_inqueue(session), message);
    }
}

STATIC void cmd_quit(Scrollback *scrollback, Settings *settings,  Session *session, char **tokens, int tkCount) {

    if (tkCount == 1) {

        add_message(session_get_inqueue(session), "QUIT");
    }
    else {
        char *msgTokens[] = {"QUIT", ":", tokens[1], tokens[2]}; 
        char message[MAX_MSG_LEN + 1] = {'\0'};

        concat_tokens(message, MAX_MSG_LEN + 1, msgTokens, ARR_SIZE(msgTokens), " ");

        add_message(session_get_inqueue(session), message);
    }
}

STATIC void cmd_unknown(Scrollback *scrollback, Settings *settings, Session *session, char **tokens, int tkCount) {

    char message[MAX_MSG_LEN + 1] = {'\0'};
    concat_tokens(message, MAX_MSG_LEN + 1, tokens, tkCount, " ");

    display_response(scrollback, "Unknown command: %s", &message[1]);
}

