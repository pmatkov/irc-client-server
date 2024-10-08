#ifdef TEST
#include "priv_command_handler.h"
#include "../../shared/src/mock.h"
#else
#include "command_handler.h"
#endif

#include "display.h"
#include "../../shared/src/settings.h"
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

#ifdef MAX_CHARS
#undef MAX_CHARS
#define MAX_CHARS 509
#endif


#define CRLF_LEN 2
#define LEAD_CHAR_LEN 1
#define MAX_NICKNAME_LEN 9

STATIC void cmd_help(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_connect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_disconnect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_nick(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_user(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_join(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_part(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_quit(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_unknown(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);

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


STATIC void cmd_help(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_commands(scrollback, get_commands(), COMMAND_TYPE_COUNT);
    } 
    else {

        CommandType commandType = string_to_command_type(cmdTokens->args[0]);

        if (commandType == UNKNOWN_COMMAND_TYPE || cmdTokens->argCount > 1) {
            cmd_unknown(scrollback, tcpClient, cmdTokens);
        }
        else {
            display_usage(scrollback, get_command(commandType));
        }
    }
}

STATIC void cmd_connect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (is_client_connected(tcpClient)) {

        display_response(scrollback, "Already connected");
    }
    else {

        // set default address and/ or port
        char *address = cmdTokens->args[0] == NULL ? DEFAULT_ADDRESS : (char*) cmdTokens->args[0];
        char *port = cmdTokens->args[1] == NULL ? DEFAULT_PORT : (char*) cmdTokens->args[1];

        int connStatus = client_connect(tcpClient, address, port);

        if (connStatus == 0) {

            char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

            // NICK <nickname>
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {"NICK"}, {get_property_value(NICKNAME)}, 0});
            add_message_to_client_queue(tcpClient, message);

            char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
            get_client_ip(ipv4Address, sizeof(ipv4Address), get_socket_fd(tcpClient));

            memset(message, '\0', sizeof(message));

            // USER <username> <hostname> <servername> <real name>
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {"USER", get_property_value(USERNAME), ipv4Address, "*"}, {get_property_value(REALNAME)}, 1});

            add_message_to_client_queue(tcpClient, message);
            display_response(scrollback, "Connected to %s:%s.", address, port);
        }
        else if (connStatus == -1) {
            display_response(scrollback, "Unable to connect to %s:%s.", address, port);
        }
        else if (connStatus == -2) {
            display_response(scrollback, "Invalid address: %s.", address);
        } 
        else if (connStatus == -3) {
            display_response(scrollback, "Invalid port: %s.", port);
        }
    }
}

STATIC void cmd_disconnect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(scrollback, "Not connected.");
    }
    else {

        if (!cmdTokens->argCount) {

            add_message_to_client_queue(tcpClient, (char*) cmdTokens->command);
        }
        else {

            char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};
            char notice[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

            concat_tokens(notice, MAX_CHARS, cmdTokens->args, ARR_SIZE(notice), " ");

            // DISCONNECT [:message]
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command}, {notice}, 1});

            add_message_to_client_queue(tcpClient, message);
            display_response(scrollback, "Disconnected.");
        }
    }
}

STATIC void cmd_nick(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Missing argument <nickname>.");
    }
    else if (cmdTokens->argCount > 1 || strlen(cmdTokens->args[0]) > MAX_NICKNAME_LEN) {
        display_response(scrollback, "Nickname is a single word of maximum 9 chars.");
    }
    else {

        char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

        // NICK <nickname>
        create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});
        add_message_to_client_queue(tcpClient, message);

        set_property_value(NICKNAME, cmdTokens->args[0]);
        display_response(scrollback, "Nickname set to: %s.", cmdTokens->args[0]);
    }
}

STATIC void cmd_user(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Not enough arguments.");
    }
    else {

        char realName[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};
        const char *tokens[] = {cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}; 

        if (cmdTokens->argCount) {

            set_property_value(USERNAME, cmdTokens->args[0]);
        }
        if (cmdTokens->argCount > 1) {
            
            concat_tokens(realName, MAX_CHARS, tokens, ARR_SIZE(tokens), " ");
            set_property_value(REALNAME, realName);
        }
        // USER <username> [real name]
        display_response(scrollback, "Username set to: %s. Real name set to: %s", get_property_value(USERNAME), get_property_value(REALNAME));
    }
}

STATIC void cmd_join(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(scrollback, "Unable to join. Not connected.");
    }
    else {

        if (!cmdTokens->argCount) {

            display_response(scrollback, "Missing argument <channel>.");
        }
        else if (cmdTokens->argCount == 1) {

            char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

            // JOIN <channel>
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});
            add_message_to_client_queue(tcpClient, message);
        }
        else {

            cmd_unknown(scrollback, tcpClient, cmdTokens);
        }
    }
}

STATIC void cmd_part(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(scrollback, "Unable to leave. Not connected.");
    }
    else {
        char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

        if (!cmdTokens->argCount) {

            display_response(scrollback, "Missing argument <channel>.");
        }
        else if (cmdTokens->argCount == 1) {

            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command, cmdTokens->args[0]}, {NULL}, 0});
            add_message_to_client_queue(tcpClient, message);

        }
        else {

            char notice[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

            const char *tokens[] = {cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}; 

            concat_tokens(notice, MAX_CHARS, tokens, ARR_SIZE(notice), " ");

            // PART <channel> [message]
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command, cmdTokens->args[0]}, {notice}, 1});
            add_message_to_client_queue(tcpClient, message);
        }
    } 
}

STATIC void cmd_privmsg(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(scrollback, "Unable to send message. Not connected.");
    }
    else {
            
        if (!cmdTokens->argCount) {

            display_response(scrollback, "Not enough arguments.");

        }
        else if (cmdTokens->argCount == 1) {

            display_response(scrollback, "Missing argument <message>.");
        }
        else {

            char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};
            char notice[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

            const char *tokens[] = {cmdTokens->args[1], cmdTokens->args[2], cmdTokens->args[3], cmdTokens->args[4]}; 

            concat_tokens(notice, MAX_CHARS, tokens, ARR_SIZE(notice), " ");

            // PRIVMSG <channel | user> [message]
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command, cmdTokens->args[0]}, {notice}, 1});
            add_message_to_client_queue(tcpClient, message);
        }
    }
}

STATIC void cmd_quit(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (is_client_connected(tcpClient)) {

        cmd_disconnect(scrollback, tcpClient, cmdTokens);
        
    }
    exit(EXIT_SUCCESS);
}

STATIC void cmd_unknown(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};
    char notice[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

    concat_tokens(notice, MAX_CHARS, cmdTokens->args, ARR_SIZE(notice), " ");

    create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command}, {notice}, 0});

    display_response(scrollback, "Unknown command: %s", message);
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

        cmdTokens->command = &tokens[0][1];

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