#ifdef TEST
#include "priv_command_handler.h"
#include "../../libs/src/mock.h"
#else
#include "command_handler.h"
#endif

#include "display.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

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
STATIC void cmd_address(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_port(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
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
    cmd_address,
    cmd_port,
    cmd_quit,
    cmd_unknown
};

_Static_assert(sizeof(COMMAND_FUNCTIONS) / sizeof(COMMAND_FUNCTIONS[0]) == COMMAND_TYPE_COUNT, "Array size mismatch");

/* displays a list of available commands or 
    provides help with the usage of a specific 
    command */
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

/* initiates a connection to the IRC server. once
    the connection is established, the client sends
    NICK and USER messages in accordance with the
    IRC standard */
STATIC void cmd_connect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (is_client_connected(tcpClient)) {

        display_response(scrollback, "Already connected");
    }
    else {

        /* assigns default address and/ or port
            if none was provided */
        char *address = cmdTokens->args[0] == NULL ? (char*) get_property_value(ADDRESS) : (char*) cmdTokens->args[0];
        char *port = cmdTokens->args[1] == NULL ? (char*) get_property_value(PORT) : (char*) cmdTokens->args[1];

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
            display_response(scrollback, "Connected to the server at %s:%s.", address, port);
        }
        else if (connStatus == -1) {
            display_response(scrollback, "Unable to connect to the server at %s:%s.", address, port);
        }
        else if (connStatus == -2) {
            display_response(scrollback, "Invalid address: %s.", address);
        } 
        else if (connStatus == -3) {
            display_response(scrollback, "Invalid port: %s.", port);
        }
    }
}

/* disconnects the client from the server with
    an optional message */
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
            display_response(scrollback, "Disconnected from the server.");
        }
    }
}

/* sets a nickname for the user */
STATIC void cmd_nick(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Missing argument <nickname>.");
    }
    else if (cmdTokens->argCount > 1 || strlen(cmdTokens->args[0]) > MAX_NICKNAME_LEN) {
        display_response(scrollback, "Nickname is a single word of maximum 9 chars.");
    }
    else {

        if (is_client_connected(tcpClient)) {

            char message[MAX_CHARS + CRLF_LEN + LEAD_CHAR_LEN + 1] = {'\0'};

            // NICK <nickname>
            create_message(message, MAX_CHARS, &(MessageTokens){{NULL}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});
            add_message_to_client_queue(tcpClient, message);
        }

        set_property_value(NICKNAME, cmdTokens->args[0]);
        display_response(scrollback, "Nickname is set to: %s.", cmdTokens->args[0]);
    }
}

/* sets a username and a real name for 
    the user */
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
        display_response(scrollback, "Username is set to: %s. Real name is set to: %s", get_property_value(USERNAME), get_property_value(REALNAME));
    }
}

/* joins the client to a channel. if there
    is not already a channel with the provided
    name, a new channel will be created */
STATIC void cmd_join(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(scrollback, "Unable to join the channel. Not connected.");
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

/* removes the client from a channel */
STATIC void cmd_part(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(scrollback, "Unable to leave the channel. Not connected.");
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

/* sends a message to another user on the
    server or to the channel */
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

/* sets default ip address of the
    server */
STATIC void cmd_address(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Not enough arguments.");
    }
    else if (cmdTokens->argCount) {

        set_property_value(ADDRESS, cmdTokens->args[0]);
    }
}

/* sets default port of the server */
STATIC void cmd_port(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!cmdTokens->argCount) {
        display_response(scrollback, "Not enough arguments.");
    }
    else if (cmdTokens->argCount) {

        set_property_value(PORT, cmdTokens->args[0]);
    }
}

/* disconnects the client from the server
    and quits the app */
STATIC void cmd_quit(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (is_client_connected(tcpClient)) {

        cmd_disconnect(scrollback, tcpClient, cmdTokens);
        
    }
    exit(EXIT_SUCCESS);
}

/* informs the user that an unknown command
    was parsed */
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

void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens) {

    if (lnEditor == NULL || cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = get_current_item(le_get_buffer(lnEditor));
    set_char_in_message(message, '\0', le_get_char_count(lnEditor), get_reg_message_content);

    char *input = get_reg_message_content(message);
    strcpy(cmdTokens->input, input);

    /* valid commands start with the
         '/' prefix */

    if (has_command_prefix(input)) {

        /* the input string is split into tokens
            and saved in the CommandsTokens structure
            for command parsing */

        const char *tokens[MAX_TOKENS] = {NULL};

        int tkCount = split_input_string(cmdTokens->input, tokens, MAX_TOKENS, ' ');

        cmdTokens->command = &tokens[0][1];

        for (int i = 0; i < tkCount - 1; i++) {
            cmdTokens->args[i] = tokens[i+1];
        }

        cmdTokens->argCount = tkCount - 1; 
    }

    /* after tokenizing, the input string is 
        deleted from the "input window" and
        the line editor is reset to its default
        values. additionally, the input string
        is stored in the command buffer for later
        access (command history) */

    delete_part_line(le_get_window(lnEditor), PROMPT_SIZE);

    le_set_char_count(lnEditor, 0);
    le_set_cursor(lnEditor, PROMPT_SIZE);

    enqueue(le_get_buffer(lnEditor), message);
}