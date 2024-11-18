#ifdef TEST
#include "priv_command_handler.h"
#include "../../libs/src/mock.h"
#else
#include "command_handler.h"
#endif

#include "main.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/irc_message.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

STATIC void cmd_help(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_connect(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_disconnect(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_nick(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_user(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_join(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_part(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_address(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_port(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_quit(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_unknown(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

/* a list of command functions */
static const CommandFunc COMMAND_FUNCTIONS[] = {
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

static_assert(ARR_SIZE(COMMAND_FUNCTIONS) == COMMAND_TYPE_COUNT, "Array size mismatch");

/* parse command line input */
void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens) {

    if (lnEditor == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    RegMessage *message = get_current_item(le_get_cmd_queue(lnEditor));
    set_char_in_message(message, '\0', le_get_char_count(lnEditor), get_reg_message_content);

    char *input = get_reg_message_content(message);
    strcpy(get_command_input(cmdTokens), input);

    /* valid commands are prefixed with '/'  */
    if (has_command_prefix(input)) {

        /* a command is divided into tokens and
            stored in the commandTokens structure */
        const char *tokens[MAX_TOKENS] = {NULL};

        int tkCount = tokenize_string(get_command_input(cmdTokens), tokens, MAX_TOKENS, " ");

        set_command(cmdTokens, &tokens[0][1]);

        for (int i = 0; i < tkCount - 1; i++) {

            // cmdTokens->args[i] = tokens[i+1];
            set_command_argument(cmdTokens, tokens[i + 1], i);
        }

        set_command_argument_count(cmdTokens, tkCount - 1);
    }

    /* after parsing, a command is removed from
        the "input window" and the lineEditor
        is reset to its default values. a number
        of commands is saved for later access via
        command history */
    delete_part_line(le_get_window(lnEditor), PROMPT_SIZE);

    le_set_char_count(lnEditor, 0);
    le_set_cursor(lnEditor, PROMPT_SIZE);

    enqueue(le_get_cmd_queue(lnEditor), message);
}

/* display a list of available commands and 
    provide usage instructions */
STATIC void cmd_help(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    // CommandType commandType = string_to_command_type(get_command(cmdTokens));

    CommandType commandType = string_to_command_type(get_command_argument(cmdTokens, 0));

    if (!get_command_argument_count(cmdTokens)) {

        // HELP
        display_commands(windowManager, get_command_infos(), COMMAND_TYPE_COUNT);
    } 
    else if (get_command_argument_count(cmdTokens) == 1 && commandType != UNKNOWN_COMMAND_TYPE) {

        // HELP [command name]
        display_usage(windowManager, get_command_info(commandType));
    }
    else {
        cmd_unknown(windowManager, tcpClient, cmdTokens);
    }
    
}

/* establish a connection to the IRC server */
STATIC void cmd_connect(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (is_client_connected(tcpClient)) {

        display_response(windowManager, "Already connected");
    }
    else {

        /* assign default address and/ or port if 
            no argument was provided */
        const char *address = NULL;
        int port = 0;

        if (get_command_argument(cmdTokens, 0) == NULL) {
            address = get_char_option_value(OT_SERVER_ADDRESS);
        }
        else {
            address = get_command_argument(cmdTokens, 0);
        }

        if (get_command_argument(cmdTokens, 1) == NULL) {
            port = get_int_option_value(OT_SERVER_PORT);
        }
        else {
            port = str_to_uint(get_command_argument(cmdTokens, 1));
        }

        int connStatus = client_connect(tcpClient, address, port);

        if (connStatus == 0) {

            display_status(windowManager, "[%s]  [%s]", (const char *)get_char_option_value(OT_NICKNAME), get_servername(tcpClient));

            char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            // NICK <nickname>
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{"NICK"}, {(const char *) get_char_option_value(OT_NICKNAME)}, 0, NULL, NULL});
            add_message_to_client_queue(tcpClient, message);

            char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
            get_local_address(ipv4Address, sizeof(ipv4Address), NULL, get_socket_fd(tcpClient));

            memset(message, '\0', sizeof(message));

            // USER <username> <hostname> <servername> <:real name>
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{"USER", (const char *) get_char_option_value(OT_USERNAME), ipv4Address, "*"}, {(const char *) get_char_option_value(OT_REALNAME)}, 1, NULL, NULL});

            add_message_to_client_queue(tcpClient, message);
            display_response(windowManager, "Connected to the server at %s:%d.", address, port);
        }
        else if (connStatus == -1) {
            display_response(windowManager, "Unable to connect to the server at %s:%d.", address, port);
        }
        else if (connStatus == -2) {
            display_response(windowManager, "Invalid address: %s.", address);
        } 
        else if (connStatus == -3) {
            display_response(windowManager, "Invalid port: %d.", port);
        }
    }
}

/* disconnect from the server with an
     optional message */
STATIC void cmd_disconnect(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(windowManager, "Not connected.");
    }
    else {

        client_disconnect(tcpClient);
        display_status(windowManager, "");

        if (!get_command_argument_count(cmdTokens)) {

            add_message_to_client_queue(tcpClient, (char*) get_command(cmdTokens));
        }
        else {

            char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
            char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            concat_tokens(notice, MAX_CHARS, get_command_arguments(cmdTokens), get_command_argument_count(cmdTokens), " ");

            // DISCONNECT [:message]
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {notice}, 1, NULL, NULL});

            add_message_to_client_queue(tcpClient, message);
            display_response(windowManager, "Disconnected from the server.");
        }
    }
}

/* set user nickname */
STATIC void cmd_nick(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Missing argument <nickname>.");
    }
    else if (get_command_argument_count(cmdTokens) > 1 || strlen(get_command_argument(cmdTokens, 0)) > MAX_NICKNAME_LEN) {
        display_response(windowManager, "Nickname is a single word of maximum 9 chars.");
    }
    else {

        if (is_client_connected(tcpClient)) {

            char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            // NICK <nickname>
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, NULL, NULL});
            add_message_to_client_queue(tcpClient, message);
        }

        set_option_value(OT_NICKNAME, (char*) get_command_argument(cmdTokens, 0));
        display_response(windowManager, "Nickname is set to: %s.", get_command_argument(cmdTokens, 0));
    }
}

/* set user's username and a real name */
STATIC void cmd_user(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Not enough arguments.");
    }
    else {

        if (get_command_argument_count(cmdTokens)) {

            set_option_value(OT_USERNAME, (char*) get_command_argument(cmdTokens, 0));
        }
        if (get_command_argument_count(cmdTokens) > 1) {

            char realName[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
            const char *tokens[] = {get_command_argument(cmdTokens, 0), get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3)}; 

            concat_tokens(realName, MAX_CHARS, &tokens[1], ARR_SIZE(tokens) - 1, " ");

            set_option_value(OT_REALNAME, realName);
        }
        // USER <username> [real name]
        display_response(windowManager, "Username is set to: %s. Real name is set to: %s", (const char *)get_char_option_value(OT_USERNAME), (const char *)get_char_option_value(OT_REALNAME));
    }
}

/* join a channel */
STATIC void cmd_join(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(windowManager, "Unable to join the channel. Not connected.");
    }
    else {

        if (!get_command_argument_count(cmdTokens)) {

            display_response(windowManager, "Missing argument <channel>.");
        }
        else if (get_command_argument_count(cmdTokens) == 1) {

            char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            // JOIN <channel>
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, NULL, NULL});
            add_message_to_client_queue(tcpClient, message);
        }
        else {

            cmd_unknown(windowManager, tcpClient, cmdTokens);
        }
    }
}

/* leave the channel */
STATIC void cmd_part(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(windowManager, "Unable to leave the channel. Not connected.");
    }
    else {
        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        if (!get_command_argument_count(cmdTokens)) {

            display_response(windowManager, "Missing argument <channel>.");
        }
        else if (get_command_argument_count(cmdTokens) == 1) {

            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {NULL}, 0, NULL, NULL});
            add_message_to_client_queue(tcpClient, message);

        }
        else {

            char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
            const char *tokens[] = {get_command_argument(cmdTokens, 0), get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3)}; 

            concat_tokens(notice, MAX_CHARS, &tokens[1], ARR_SIZE(tokens) - 1, " ");

            // PART <channel> [message]
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {notice}, 1, NULL, NULL});
            add_message_to_client_queue(tcpClient, message);
        }
    } 
}

/* send a message to the user or to the 
    channel */
STATIC void cmd_privmsg(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_client_connected(tcpClient)) {

        display_response(windowManager, "Unable to send message. Not connected.");
    }
    else {
            
        if (!get_command_argument_count(cmdTokens)) {

            display_response(windowManager, "Not enough arguments.");

        }
        else if (get_command_argument_count(cmdTokens) == 1) {

            display_response(windowManager, "Missing argument <message>.");
        }
        else {

            char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
            char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            const char *tokens[] = {get_command_argument(cmdTokens, 0), get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3)}; 

            concat_tokens(notice, MAX_CHARS, &tokens[1], ARR_SIZE(tokens) - 1, " ");

            // PRIVMSG <channel | user> [message]
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {notice}, 1, NULL, NULL});
            add_message_to_client_queue(tcpClient, message);
        }
    }
}

/* set server's ip address */
STATIC void cmd_address(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Not enough arguments.");
    }
    else if (get_command_argument_count(cmdTokens)) {

        set_option_value(OT_SERVER_ADDRESS, (char*) get_command_argument(cmdTokens, 0));
    }
}

/* set server's port */
STATIC void cmd_port(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Not enough arguments.");
    }
    else if (get_command_argument_count(cmdTokens)) {

        int port = str_to_uint(get_command_argument(cmdTokens, 0));
        set_option_value(OT_SERVER_PORT, &port);
    }
}

/* disconnect from the server and quit */
STATIC void cmd_quit(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (is_client_connected(tcpClient)) {

        cmd_disconnect(windowManager, tcpClient, cmdTokens);
        
    }
    exit(EXIT_SUCCESS);
}

STATIC void cmd_unknown(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
    char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    concat_tokens(notice, MAX_CHARS, get_command_arguments(cmdTokens), get_command_argument_count(cmdTokens), " ");

    create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {notice}, 0, NULL, NULL});

    display_response(windowManager, "Unknown command: %s", message);
}

CommandFunc get_command_function(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMAND_FUNCTIONS[commandType];
    }
    return cmd_unknown;
}
