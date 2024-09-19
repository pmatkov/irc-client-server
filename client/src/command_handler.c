#ifdef TEST
#include "priv_command_handler.h"
#else
#include "command_handler.h"
#endif

#include "display.h"
#include "../../shared/src/priv_message.h"
#include "../../shared/src/network_utils.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

STATIC void cmd_help(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_connect(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_disconnect(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_nick(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_user(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_join(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_part(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_quit(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_unknown(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens);

// STATIC int has_command_prefix(const char *string);
STATIC void create_notice(char *buffer, int size, const char **tokens, int tkCount);

static const CommandFunction COMMAND_FUNCTIONS[] = {
    cmd_help,
    cmd_connect,
    cmd_disconnect,
    cmd_nick,
    cmd_user,
    cmd_join,
    cmd_part,
    cmd_privmsg,
    cmd_quit,
    cmd_unknown
};

_Static_assert(sizeof(COMMAND_FUNCTIONS) / sizeof(COMMAND_FUNCTIONS[0]) == COMMAND_TYPE_COUNT, "Array size mismatch");


STATIC void cmd_help(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_commands(scrollback, get_commands(), COMMAND_TYPE_COUNT);
    } 
    else {

        CommandType commandType = string_to_command_type(cmdTokens->args[0]);

        if (commandType == UNKNOWN_COMMAND_TYPE || cmdTokens->argCount > 1) {
            cmd_unknown(scrollback, settings, tcpClient, cmdTokens);
        }
        else {
            display_usage(scrollback, get_command(commandType));
        }
    }
}

STATIC void cmd_connect(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    // set default address and/ or port
    if (cmdTokens->args[0] == NULL) {
        cmdTokens->args[0] = DEFAULT_ADDRESS;

    }
    if (cmdTokens->args[1] == NULL) {
        cmdTokens->args[1] = DEFAULT_PORT;
    }

    if (client_is_connected(tcpClient)) {

        display_response(scrollback, "Already connected to: %s: %s.", cmdTokens->args[0], cmdTokens->args[1]);
    }
    else {

        int connStatus = connect_to_server(tcpClient, (char *)cmdTokens->args[0], (char *)cmdTokens->args[1]);

        if (connStatus == 0) {

            char message[MAX_CHARS + 1] = {'\0'};

            // create NICK message
            const char *nickMsgTokens[] = {"NICK", get_property_value(settings, NICKNAME)};  

            concat_tokens(message, MAX_CHARS, nickMsgTokens, ARR_SIZE(nickMsgTokens), " ");
            add_message_to_client_queue(tcpClient, message);

            // create USER message
            char *username = get_property_value(settings, USERNAME);

            char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
            get_localhost_ip(ipv4Address, sizeof(ipv4Address), client_get_fd(tcpClient));

            char realName[MAX_CHARS + 1] = {'\0'};
            int maxRealNameLen = MAX_CHARS - strlen("USER") - strlen(username) - strlen(ipv4Address) - 1;

            prepend_char(realName, maxRealNameLen, get_property_value(settings, REALNAME), ':');

            const char *userMsgTokens[] = {"USER", username, ipv4Address, "*", realName};  

            memset(message, '\0', MAX_CHARS + 1);
            concat_tokens(message, MAX_CHARS, userMsgTokens, ARR_SIZE(userMsgTokens), " ");

            add_message_to_client_queue(tcpClient, message);
        
            display_response(scrollback, "Connected to %s:%s.", cmdTokens->args[0], cmdTokens->args[1]);

        }
        else if (connStatus == -1) {
            display_response(scrollback, "Unable to connect to %s:%s.", cmdTokens->args[0], cmdTokens->args[1]);
        }
        else if (connStatus == -2) {
            display_response(scrollback, "Invalid address: %s.", cmdTokens->args[0]);
        } 
        else if (connStatus == -3) {
            display_response(scrollback, "Invalid port: %s.", cmdTokens->args[1]);
        }
    }
}

STATIC void cmd_disconnect(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {


    if (!client_is_connected(tcpClient) && strcmp(cmdTokens->command, "DISCONNECT") == 0) {

        display_response(scrollback, "Already disconnected.");
    }
    else {

        if (!cmdTokens->argCount) {

            add_message_to_client_queue(tcpClient, "DISCONNECT");
        }
        else {

            char noticeWithPrefix[MAX_CHARS + 1] = {'\0'};

            create_notice(noticeWithPrefix, MAX_CHARS - strlen("DISCONNECT") - 1, cmdTokens->args, cmdTokens->argCount);

            char message[MAX_CHARS + 1] = {'\0'};
            const char *msgTokens[] = {"DISCONNECT", noticeWithPrefix}; 
            
            concat_tokens(message, MAX_CHARS, msgTokens, ARR_SIZE(msgTokens), " ");

            add_message_to_client_queue(tcpClient, message);
        }
    }
}

STATIC void cmd_nick(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Argument <nickname> is missing.");
    }
    else if (cmdTokens->argCount > 1 || strlen(cmdTokens->args[0]) > 9) {
        display_response(scrollback, "Nickname is a single word of maximum 9 chars.");
    }
    else {

        set_property_value(settings, NICKNAME, cmdTokens->args[0]);

        display_response(scrollback, "Nickname set to: %s.", cmdTokens->args[0]);

        char message[MAX_CHARS + 1] = {'\0'};
        const char *msgTokens[] = {"NICK", cmdTokens->args[0]}; 
        
        concat_tokens(message, MAX_CHARS, msgTokens, ARR_SIZE(msgTokens), " ");

        add_message_to_client_queue(tcpClient, message);
    }
}

STATIC void cmd_user(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Not enough arguments.");
    }
    else {

        if (cmdTokens->argCount == 2) {

            set_property_value(settings, USERNAME, cmdTokens->args[0]);
        }
        if (cmdTokens->argCount > 2) {

            char realName[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}; 
            
            concat_tokens(realName, MAX_CHARS, tokens, ARR_SIZE(tokens), " ");

            set_property_value(settings, REALNAME, realName);
        }

        display_response(scrollback, "Username set to: %s. Real name set to: %s", get_property_value(settings, USERNAME), get_property_value(settings, REALNAME));
    }
}

STATIC void cmd_join(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!client_is_connected(tcpClient)) {

        display_response(scrollback, "Unable to join channel. Not connected.");
    }
    else if (client_is_inchannel(tcpClient)) {

        display_response(scrollback, "Already in the channel.");
    }
    else {

        if (!cmdTokens->argCount) {

            display_response(scrollback, "Argument <channel> is missing.");
        }
        else if (cmdTokens->argCount == 1) {

            char message[MAX_CHARS + 1] = {'\0'};
            const char *msgTokens[] = {"JOIN", cmdTokens->args[0]}; 

            concat_tokens(message, MAX_CHARS, msgTokens, ARR_SIZE(msgTokens), " ");

            add_message_to_client_queue(tcpClient, message);
        }
        else {

            cmd_unknown(scrollback, settings, tcpClient, cmdTokens);
        }
    }
}

STATIC void cmd_part(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!client_is_connected(tcpClient)) {

        display_response(scrollback, "Unable to leave the channel. Not connected.");
    }
    else if (!client_is_inchannel(tcpClient)) {

        display_response(scrollback, "Not in the channel.");
    }
    else {
        if (!cmdTokens->argCount) {

            display_response(scrollback, "Argument <channel> is missing.");
        }
        else {

            char noticeWithPrefix[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}; 

            create_notice(noticeWithPrefix, MAX_CHARS - strlen("PART") - 1, tokens, ARR_SIZE(tokens));

            char message[MAX_CHARS + 1] = {'\0'};
            const char *msgTokens[] = {"PART", cmdTokens->args[0], noticeWithPrefix}; 

            concat_tokens(message, MAX_CHARS, msgTokens, ARR_SIZE(msgTokens), " ");

            add_message_to_client_queue(tcpClient, message);
        }
    } 
}

STATIC void cmd_privmsg(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!client_is_connected(tcpClient)) {

        display_response(scrollback, "Unable to send message. Not connected.");
    }
    else {
            
        if (!cmdTokens->argCount) {

            display_response(scrollback, "Not enough arguments.");

        }
        else if (cmdTokens->argCount == 1) {

            display_response(scrollback, "Argument <message> is missing.");
        }
        else {

            char noticeWithPrefix[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}; 

            create_notice(noticeWithPrefix, MAX_CHARS - strlen("PRIVMSG") - 1, tokens, ARR_SIZE(tokens));

            char message[MAX_CHARS + 1] = {'\0'};
            const char *msgTokens[] = {"PRIVMSG", cmdTokens->args[0], noticeWithPrefix};

            concat_tokens(message, MAX_CHARS, msgTokens, ARR_SIZE(msgTokens), " ");

            add_message_to_client_queue(tcpClient, message);
        }
    }
}

STATIC void cmd_quit(Scrollback *scrollback, Settings *settings,  TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (client_is_connected(tcpClient)) {

        cmd_disconnect(scrollback, settings, tcpClient, cmdTokens);
        
    }
    exit(EXIT_SUCCESS);
}

STATIC void cmd_unknown(Scrollback *scrollback, Settings *settings, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};

    concat_tokens(message, MAX_CHARS, (const char *[]) {cmdTokens->command, cmdTokens->args[0], cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}, cmdTokens->argCount + 1, " ");

    display_response(scrollback, "Unknown command: %s", &message[1]);
}

CommandFunction get_command_function(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMAND_FUNCTIONS[commandType];
    }
    return cmd_unknown;
}

// parse command line input
void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens) {

    if (lnEditor == NULL || cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = get_current_item(le_get_buffer(lnEditor));
    set_char_in_message(message, '\0', le_get_char_count(lnEditor), get_reg_message_content);

    char *input = get_reg_message_content(message);
    strcpy(cmdTokens->input, input);

    if (has_command_prefix(input)) {

        const char *tokens[MAX_TOKENS] = {NULL};

        int tkCount = split_input_string(cmdTokens->input, tokens, MAX_TOKENS, ' ');

        cmdTokens->command = tokens[0];

        for (int i = 0; i < tkCount - 1; i++) {
            cmdTokens->args[i] = tokens[i+1];
        }

        cmdTokens->argCount = tkCount - 1; 
    }

    delete_part_line(le_get_window(lnEditor), PROMPT_SIZE);

    le_set_char_count(lnEditor, 0);
    le_set_cursor(lnEditor, PROMPT_SIZE);

    enqueue(le_get_buffer(lnEditor), message);
}

STATIC void create_notice(char *buffer, int size, const char **tokens, int tkCount) {

    char notice[MAX_CHARS + 1] = {'\0'};

    concat_tokens(notice, size, tokens, tkCount, " ");

    prepend_char(buffer, size, notice, ':');
}