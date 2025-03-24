#ifdef TEST
#include "priv_command_handler.h"
#include "../../libs/src/mock.h"
#else
#include "command_handler.h"
#endif

#include "config.h"
#include "../../libs/src/common.h"
#include "../../libs/src/session_state.h"
#include "../../libs/src/message.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/irc_message.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/enum_utils.h"
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

STATIC void parse_string(const char *string, CommandTokens *cmdTokens);
STATIC void set_uppercase_command(char *result, int size, const char *string);
STATIC void set_connection_params(CommandTokens *cmdTokens, char *ipv4address, int *port);
STATIC void register_connection(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void set_nickname(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void set_user_data(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

STATIC void cmd_help(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_connect(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_disconnect(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_nick(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_user(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_join(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_part(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_address(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_port(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_whois(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_quit(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void cmd_unknown(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

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
    cmd_whois,
    cmd_quit,
    cmd_unknown
};

ASSERT_ARRAY_SIZE(COMMAND_FUNCTIONS, COMMAND_TYPE_COUNT)

void parse_cli_input(InputWindow *inputWindow, CommandTokens *cmdTokens) {

    if (inputWindow == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *inputBaseWindow = get_le_base_window(inputWindow);
    LineEditor *lnEditor = get_line_editor(inputWindow);

    Message *message = get_current_item(get_le_front_cmd_queue(lnEditor));
    const char *content = get_message_content(message);

    set_message_char(message, '\0', strlen(content));

    /* each command is prefixed with '/' char */
    if (has_command_prefix(content)) {
        parse_string(content, cmdTokens);
    }

    delete_part_line(get_window(inputBaseWindow), PROMPT_SIZE);

    set_le_char_count(lnEditor, 0);
    set_le_cursor(lnEditor, PROMPT_SIZE);

    /* parsed commands are added to the queue for later
        access via command history */
    enqueue(get_le_back_cmd_queue(lnEditor), message);
    enqueue(get_le_front_cmd_queue(lnEditor), message);
    reload_saved_commands(lnEditor);
}

STATIC void parse_string(const char *string, CommandTokens *cmdTokens) {

    if (string == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* split the input string into tokens consisting of
    a command token and argument tokens.

    Example:
    Input: "/HELP privmsg"
    Command: "HELP"
    Arg1: "privmsg" */

    char buffer[MAX_CHARS + 1] = {'\0'};

    set_uppercase_command(buffer, ARRAY_SIZE(buffer), string);

    safe_copy(get_command_input(cmdTokens), MAX_CHARS, buffer);

    const char *tokens[MAX_TOKENS] = {NULL};

    int tkCount = tokenize_string(get_command_input(cmdTokens), tokens, MAX_TOKENS, " ");

    if (tkCount) {

        set_command(cmdTokens, (char*) &tokens[0][1]);

        for (int i = 0; i < tkCount - 1; i++) {
            set_command_argument(cmdTokens, tokens[i + 1], i);
        }
        set_command_argument_count(cmdTokens, tkCount - 1);
    }
}

STATIC void set_uppercase_command(char *buffer, int size, const char *string) {

    if (buffer == NULL || string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    safe_copy(buffer, size, string);

    int nextWordIdx = strchr(buffer, ' ') - buffer;

    strn_to_upper(buffer, size, string, nextWordIdx);
}

/* display a list of available commands or 
    provide help for a specific command */
STATIC void cmd_help(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), HELP)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    // HELP [command name]
    if (!get_command_argument_count(cmdTokens)) {

        display_commands(windowManager, get_cmd_infos(), COMMAND_TYPE_COUNT - 1);
    } 
    else if (get_command_argument_count(cmdTokens) == 1) {

        CommandType cmdType = string_to_command_type(get_command_argument(cmdTokens, 0));

        if (cmdType != UNKNOWN_COMMAND_TYPE) {

            display_usage(windowManager, get_cmd_info(cmdType));
        }
        else {
            cmd_unknown(eventManager, windowManager, tcpClient, cmdTokens);
        }
    }
    else {
        cmd_unknown(eventManager, windowManager, tcpClient, cmdTokens);
    }
}

/* establish a connection to the server */
STATIC void cmd_connect(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    const SessionState **sessionStates = get_client_session_states();

    if (!is_allowed_state_command(sessionStates, get_client_state_type(tcpClient), CONNECT)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    char ipv4address[INET_ADDRSTRLEN] = {'\0'};
    int port = 0;

    set_connection_params(cmdTokens, ipv4address, &port);

    int connStatus = client_connect(tcpClient, eventManager, ipv4address, port);

    if (!connStatus) {

        register_connection(eventManager, windowManager, tcpClient, cmdTokens);

        BaseWindow *statusWindow = get_status_window(windowManager);
        InputWindow *inputWindow = get_input_window(windowManager);

        display_status(statusWindow, inputWindow, &(StatusParams){MAIN_STATUS, 0, 0, 0, {"", ""}, "[%s]  [%s]", 0}, get_char_option_value(OT_NICKNAME), get_server_identifier(tcpClient));
        display_response(windowManager, "Connecting to the server at %s:%d.", ipv4address, port);

    }
    else if (connStatus == -1) {
        display_response(windowManager, "Unable to connect to the server at %s:%d.", ipv4address, port);
    }
    else if (connStatus == -2) {
        display_response(windowManager, "Invalid address: %s.", ipv4address);
    } 
    else if (connStatus == -3) {
        display_response(windowManager, "Invalid port: %d.", port);
    }
}

STATIC void set_connection_params(CommandTokens *cmdTokens, char *ipv4address, int *port) {

    if (cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *address = NULL;

    if (get_command_argument(cmdTokens, 0) != NULL) {
        address = get_command_argument(cmdTokens, 0);
    }
    else {
        address = get_char_option_value(OT_SERVER_ADDRESS);
    }

    if (is_valid_ip(address)) {
        safe_copy(ipv4address, INET_ADDRSTRLEN, address);
    }
    else {
        hostname_to_ip(ipv4address, INET_ADDRSTRLEN, address);
    }

    if (get_command_argument(cmdTokens, 1) != NULL) {
        *port = str_to_uint(get_command_argument(cmdTokens, 1));
    }
    else {
        *port = get_int_option_value(OT_SERVER_PORT);
    }
}

STATIC void register_connection(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    // NICK <nickname>
    char command[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    const char *tokens1[] = {"/NICK", get_char_option_value(OT_NICKNAME)}; 
    concat_tokens(command, MAX_CHARS, tokens1, ARRAY_SIZE(tokens1), " ");
    reset_command_tokens(cmdTokens);

    parse_string(command, cmdTokens);
    cmd_nick(eventManager, windowManager, tcpClient, cmdTokens);

    memset(command, '\0', ARRAY_SIZE(command));

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
    get_local_address(ipv4Address, sizeof(ipv4Address), NULL, get_client_fd(tcpClient));

    // USER <username> <hostname> <servername> :<realname>
    const char *tokens2[] = {"/USER", get_char_option_value(OT_USERNAME), get_char_option_value(OT_REALNAME)}; 
    concat_tokens(command, MAX_CHARS, tokens2, ARRAY_SIZE(tokens2), " ");
    reset_command_tokens(cmdTokens);

    parse_string(command, cmdTokens);
    cmd_user(eventManager, windowManager, tcpClient, cmdTokens);
}

/* disconnect from the server with an
     optional message */
STATIC void cmd_disconnect(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), DISCONNECT)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    // DISCONNECT [:message]
    if (!get_command_argument_count(cmdTokens)) {

        enqueue_to_client_queue(tcpClient, "QUIT");
    }
    else {

        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
        char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        concat_tokens(notice, MAX_CHARS, get_command_arguments(cmdTokens), get_command_argument_count(cmdTokens), " ");
        create_irc_message(message, MAX_CHARS, &(IRCMessage){{"QUIT"}, {notice}, 1, NULL, NULL});

        enqueue_to_client_queue(tcpClient, message);
    }

    BaseWindow *statusWindow = get_status_window(windowManager);
    InputWindow *inputWindow = get_input_window(windowManager);

    client_disconnect(tcpClient, eventManager);

    display_status(statusWindow, inputWindow, &(StatusParams){MAIN_STATUS, 0, 0, 0, {"", ""}, "", 0});
    display_response(windowManager, "Disconnecting from the server...");
}

/* set user's nickname */
STATIC void cmd_nick(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), NICK)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Missing argument <nickname>.");
    }
    else if (get_command_argument_count(cmdTokens) > 1 || strlen(get_command_argument(cmdTokens, 0)) > MAX_NICKNAME_LEN) {
        display_response(windowManager, "Nickname must be a single word of max 9 chars.");
    }
    else if (get_command_argument_count(cmdTokens) == 1) {
        set_nickname(windowManager, tcpClient, cmdTokens);
    }
}

STATIC void set_nickname(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    const SessionStateType clientState = get_client_state_type(tcpClient);

    if (clientState == CONNECTED || clientState == REGISTERED || clientState == IN_CHANNEL) {
    
        // NICK <nickname>
        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, NULL, NULL});
        enqueue_to_client_queue(tcpClient, message);

        if (is_allowed_state_transition(get_client_session_states(), get_client_state_type(tcpClient), START_REGISTRATION)) {
            set_client_state_type(tcpClient, START_REGISTRATION);
        }
    }

    set_option_value(OT_NICKNAME, (char*) get_command_argument(cmdTokens, 0));

    BaseWindow *statusWindow = get_status_window(windowManager);
    InputWindow *inputWindow = get_input_window(windowManager);

    if (clientState == DISCONNECTED) {
        display_status(statusWindow, inputWindow, &(StatusParams){MAIN_STATUS, 0, 0, 0, {"", ""}, "[%s]", 0}, get_char_option_value(OT_NICKNAME));
    }
    else {
        display_status(statusWindow, inputWindow, &(StatusParams){MAIN_STATUS, 0, 0, 0, {"", ""}, "[%s]  [%s]", 0}, get_char_option_value(OT_NICKNAME), get_server_identifier(tcpClient));
    }

    if (get_client_state_type(tcpClient) != START_REGISTRATION) {
        display_response(windowManager, "Nickname is set to: %s.", get_command_argument(cmdTokens, 0));
    }
}

/* set user's username and real name */
STATIC void cmd_user(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), USER)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Not enough arguments.");
    }
    else if (get_command_argument_count(cmdTokens) >= 1) {
        set_user_data(windowManager, tcpClient, cmdTokens);
    }
}

STATIC void set_user_data(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    if (get_command_argument_count(cmdTokens) == 1) {

        set_option_value(OT_USERNAME, (char*) get_command_argument(cmdTokens, 0));

        if (get_client_state_type(tcpClient) == DISCONNECTED) {
            display_response(windowManager, "Username is set to: %s.", get_char_option_value(OT_USERNAME));
        }
    }
    else if (get_command_argument_count(cmdTokens) > 1) {

        char realName[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        const char *tokens[] = {get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3), get_command_argument(cmdTokens, 4)}; 

        concat_tokens(realName, MAX_CHARS, tokens, get_command_argument_count(cmdTokens) - 1, " ");

        set_option_value(OT_REALNAME, realName);

        if (get_client_state_type(tcpClient) == DISCONNECTED) {
            display_response(windowManager, "Real name is set to: %s", get_char_option_value(OT_REALNAME));
        }
    }

    if (get_client_state_type(tcpClient) == START_REGISTRATION) {

        // USER <username> [real name]
        char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
        get_local_address(ipv4Address, sizeof(ipv4Address), NULL, get_client_fd(tcpClient));

        create_irc_message(message, MAX_CHARS, &(IRCMessage){{"USER", get_char_option_value(OT_USERNAME), ipv4Address, "*"}, {get_char_option_value(OT_REALNAME)}, 1, NULL, NULL});
        enqueue_to_client_queue(tcpClient, message);

        if (is_allowed_state_transition(get_client_session_states(), get_client_state_type(tcpClient), REGISTERED)) {
            set_client_state_type(tcpClient, REGISTERED);
        }
    }
}

/* join a channel */
STATIC void cmd_join(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    const SessionState **sessionStates = get_client_session_states();

    if (!is_allowed_state_command(sessionStates, get_client_state_type(tcpClient), JOIN)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Missing argument <channel>.");
    }
    else if (get_command_argument_count(cmdTokens) > 1) {
        display_response(windowManager, "Too many arguments.");
    }
    else if (get_command_argument_count(cmdTokens) == 1) {

        // JOIN <channel>
        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, NULL, NULL});
        enqueue_to_client_queue(tcpClient, message);

        if (is_allowed_state_transition(sessionStates, get_client_state_type(tcpClient), IN_CHANNEL)) {
            set_client_state_type(tcpClient, IN_CHANNEL);
        }
    }
}

/* leave the channel */
STATIC void cmd_part(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    const SessionState **sessionStates = get_client_session_states();

    if (!is_allowed_state_command(sessionStates, get_client_state_type(tcpClient), PART)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {

        display_response(windowManager, "Missing argument <channel>.");
    }
    else if (get_command_argument_count(cmdTokens) >= 1) {

        // PART <channel> [message]
        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        if (get_command_argument_count(cmdTokens) == 1) {

            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {NULL}, 0, NULL, NULL});
        }
        else {

            char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
            const char *tokens[] = {get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3), get_command_argument(cmdTokens, 4)}; 

            concat_tokens(notice, MAX_CHARS, tokens, get_command_argument_count(cmdTokens) - 1, " ");
            create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {notice}, 1, NULL, NULL});

        }
        enqueue_to_client_queue(tcpClient, message);
    }
}

/* send a message to the user or to the 
    channel */
STATIC void cmd_privmsg(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {


    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), PRIVMSG)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }
            
    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Not enough arguments.");
    }
    else if (get_command_argument_count(cmdTokens) == 1) {
        display_response(windowManager, "Missing argument <message>.");
    }
    else if (get_command_argument_count(cmdTokens) > 1) {

        // PRIVMSG <channel | user> [message]
        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
        char notice[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        const char *tokens[] = {get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3), get_command_argument(cmdTokens, 4)}; 

        concat_tokens(notice, MAX_CHARS, tokens, get_command_argument_count(cmdTokens) - 1, " ");

        create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {notice}, 1, NULL, NULL});
        enqueue_to_client_queue(tcpClient, message);
    }
}

/* set server's ip address */
STATIC void cmd_address(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), ADDRESS)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Missing argument <address>.");
    }
    else if (get_command_argument_count(cmdTokens) >= 1) {

        set_option_value(OT_SERVER_ADDRESS, (char*) get_command_argument(cmdTokens, 0));
    }
}

/* set server's port */
STATIC void cmd_port(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), PORT)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Missing argument <port>.");
    }
    else if (get_command_argument_count(cmdTokens) >= 1) {

        int port = str_to_uint(get_command_argument(cmdTokens, 0));
        set_option_value(OT_SERVER_PORT, &port);
    }
}

/* get info about specific user */
STATIC void cmd_whois(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), WHOIS)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {
        display_response(windowManager, "Missing argument <nickname>.");
    }
    else if (get_command_argument_count(cmdTokens) > 1) {
        display_response(windowManager, "Too many arguments.");
    }
    else if (get_command_argument_count(cmdTokens) == 1) {

        char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(message, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {NULL}, 0, NULL, NULL});
        enqueue_to_client_queue(tcpClient, message);
    }
}

/* disconnect from the server and quit */
STATIC void cmd_quit(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_client_session_states(), get_client_state_type(tcpClient), QUIT)) {
        display_response(windowManager, "Command not allowed in current state.");
        return;
    }

    cmd_disconnect(eventManager, windowManager, tcpClient, cmdTokens);

    if (eventManager != NULL) {

        push_event_to_queue(eventManager, &(Event){.eventType = NETWORK_EVENT, .subEventType = NE_CLIENT_DISCONNECT, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE});
        push_event_to_queue(eventManager, &(Event){.eventType = SYSTEM_EVENT, .subEventType = SE_EXIT, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE});
    }
    
}

STATIC void cmd_unknown(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    const char *tokens[] = {get_command(cmdTokens), get_command_argument(cmdTokens, 0), get_command_argument(cmdTokens, 1), get_command_argument(cmdTokens, 2), get_command_argument(cmdTokens, 3), get_command_argument(cmdTokens, 4)}; 
    concat_tokens(message, MAX_CHARS, tokens, get_command_argument_count(cmdTokens) + 1, " ");

    display_response(windowManager, "Unknown command: %s", message);
}

CommandFunc get_command_function(CommandType cmdType) {

    CommandFunc cmdFunc = NULL;

    if (is_valid_enum_type(cmdType, COMMAND_TYPE_COUNT) && COMMAND_FUNCTIONS[cmdType] != NULL) {
        cmdFunc = COMMAND_FUNCTIONS[cmdType];
    }
    else {
        cmdFunc = cmd_unknown;
    }

    return cmdFunc;
}
