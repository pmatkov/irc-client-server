
#ifdef TEST
#else
#include "dispatcher.h"
#endif

#include "config.h"
#include "tcp_server.h"
#include "command_handler.h"

#include "../../libs/src/command.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/enum_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MSG_TOKENS 2

typedef struct {
    EventManager *eventManager;
    PollManager *pollManager;
    TCPServer *tcpServer;
    CommandTokens *cmdTokens;
} EventContext;

static EventContext eventContext = {NULL};

STATIC void detect_pipe_event_type(const char *message, Event *event);
STATIC void execute_command(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

void process_connection_request(EventManager *eventManager, TCPServer *tcpServer) {
    
    if (eventManager == NULL || tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_CLIENT_CONNECT, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE};

    push_event_to_queue(eventManager, &event);
}

void process_pipe_data(EventManager *eventManager, StreamPipe *streamPipe) {

    if (eventManager == NULL || streamPipe == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char message[MAX_CHARS + 1] = {'\0'};
    char *pipeBuffer = get_pipe_buffer(streamPipe);

    read_string(get_pipe_fd(streamPipe, READ_PIPE), pipeBuffer + strlen(pipeBuffer), get_pipe_buffer_size(streamPipe) - strlen(pipeBuffer));

    while (extract_message(message, ARRAY_SIZE(message), pipeBuffer, CRLF)) {

        Event event = {.eventType = UNKNOWN_EVENT_TYPE, .subEventType = UNASSIGNED, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE};

        detect_pipe_event_type(message, &event);

        if (event.eventType != UNKNOWN_EVENT_TYPE) {
            push_event_to_queue(eventManager, &event);
        }
    }

    if (is_pipe_buffer_full(streamPipe)) {
        reset_pipe_buffer(streamPipe);
    }
}

void process_socket_data(EventManager *eventManager, TCPServer *tcpServer, int fd) {

    if (eventManager == NULL || tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int readStatus = server_read(tcpServer, eventManager, fd);

    if (readStatus == 1) {

        int fdIdx = find_fd_idx_in_hash_table(get_server_fds_idx_map(tcpServer), fd);
        Client *client = get_client(tcpServer, fdIdx);

        char message[MAX_CHARS + 1] = {'\0'};

        while (extract_message(message, ARRAY_SIZE(message), get_client_inbuffer(client), CRLF)) {

            char fmtMessage[MAX_CHARS + 1] = {'\0'};
            char fdStr[MAX_DIGITS] = {'\0'};

            uint_to_str(fdStr, ARRAY_SIZE(fdStr), fd);
            const char *tokens[] = {fdStr, "|", message};

            concat_tokens(fmtMessage, ARRAY_SIZE(fmtMessage), tokens, ARRAY_SIZE(tokens), "");

            Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_CLIENT_MSG, .dataItem = (DataItem) {.itemChar = ""}, .dataType = CHAR_TYPE};
            safe_copy(event.dataItem.itemChar, ARRAY_SIZE(event.dataItem.itemChar), fmtMessage);

            push_event_to_queue(eventManager, &event);
        }
    }
}

void dispatch_events(EventManager *eventManager) {

    if (eventManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Event *event = NULL;

    while ((event = pop_event_from_queue(eventManager)) != NULL) {

        if (get_event_type(event) == NETWORK_EVENT) {
            dispatch_network_event(eventManager, event);
        }
        if (get_event_type(event) == SYSTEM_EVENT) {
            dispatch_system_event(eventManager, event);
        }
    }
}

void handle_ne_client_connect_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    add_client(eventContext.tcpServer, eventContext.eventManager);
}

void handle_ne_client_disconnect_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int fdIdx = find_fd_idx_in_hash_table(get_server_fds_idx_map(eventContext.tcpServer), event->dataItem.itemInt);
    Client *client = get_client(eventContext.tcpServer, fdIdx);
    Session *session = get_session(eventContext.tcpServer);
    User *user = find_user_in_hash_table(session, get_client_nickname(client));

    // <:nickname!username@hostname> QUIT <:message>
    char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{"QUIT"}, {NULL}, 0, create_user_info, user});

    leave_all_channels(get_session(eventContext.tcpServer), user, fwdMessage);

    ReadyList *readyList = get_ready_list(session);
    remove_user_from_ready_list(get_ready_users(readyList), user);

    unregister_user(session, user);
    remove_client(eventContext.tcpServer, eventContext.eventManager, get_client_fd(client));

}

void handle_ne_remove_poll_fd_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unset_poll_fd(eventContext.pollManager, event->dataItem.itemInt);
    close(event->dataItem.itemInt);
}

void handle_ne_add_poll_fd_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    set_poll_fd(eventContext.pollManager, event->dataItem.itemInt);
}

void handle_ne_client_msg_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (count_delimiters(event->dataItem.itemChar, "|") == 1) {

        const char *tokens[MSG_TOKENS] = {NULL};
        tokenize_string(event->dataItem.itemChar, tokens, ARRAY_SIZE(tokens), "|");

        int fdIdx = find_fd_idx_in_hash_table(get_server_fds_idx_map(eventContext.tcpServer), str_to_uint(tokens[0]));
        Client *client = get_client(eventContext.tcpServer, fdIdx);

        if (get_int_option_value(OT_ECHO)) {
            add_message_to_queue(eventContext.tcpServer, client, tokens[1]);
        }
        else {
            parse_message(tokens[1], eventContext.cmdTokens);
            execute_command(eventContext.tcpServer, client, eventContext.cmdTokens);
        }
    }
}

void handle_se_exit_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    exit(EXIT_SUCCESS);
}

STATIC void detect_pipe_event_type(const char *message, Event *event) {

    if (message == NULL || event == NULL) {
        FAILED(ARG_ERROR, NULL);
    } 

    if (strcmp(message, "sigint") == 0) {
        event->eventType = SYSTEM_EVENT;
        event->subEventType = SE_EXIT;
    }
}

STATIC void execute_command(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (tcpServer == NULL || client == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *command = get_command(cmdTokens);

    if (command != NULL) {
            
        CommandType cmdType = string_to_command_type(command);

        CommandFunc commandFunc = get_command_function(cmdType);
        commandFunc(eventContext.eventManager, tcpServer, client, cmdTokens);

        reset_command_tokens(cmdTokens);
    }
}

void send_socket_messages(EventManager *eventManager, TCPServer *tcpServer) {

    if (eventManager == NULL || tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Session *session = get_session(tcpServer);

    /* send server queue messages (unregistered clients) */
    while (!is_queue_empty(get_server_out_queue(tcpServer))) {

        char *message = dequeue_from_server_queue(tcpServer);

        if (count_delimiters(message, "|") == 1) {

            const char *tokens[MSG_TOKENS] = {NULL};
            tokenize_string(message, tokens, ARRAY_SIZE(tokens), "|");

            server_write(tcpServer, eventManager, str_to_uint(tokens[0]), tokens[1]);
        }
    }

    struct {
        TCPServer *tcpServer;
        EventManager *eventManager;
        const char *message;
    } data = {tcpServer, eventManager, NULL};

    /* send messages from users' and channels' queues ( 
        users and channels have dedicated queues) */
    iterate_list(get_ready_users(get_ready_list(session)), send_user_queue_messages, &data);
    iterate_list(get_ready_channels(get_ready_list(session)), send_channel_queue_messages, &data);

    reset_linked_list(get_ready_users(get_ready_list(session)));
    reset_linked_list(get_ready_channels(get_ready_list(session)));
}

void set_event_context(EventManager *eventManager, PollManager *pollManager, TCPServer *tcpServer, CommandTokens *cmdTokens) {

    if (eventManager == NULL || pollManager == NULL || tcpServer == NULL  || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    eventContext.eventManager = eventManager;
    eventContext.pollManager = pollManager;
    eventContext.tcpServer = tcpServer;
    eventContext.cmdTokens = cmdTokens;
}

void register_event_handlers(EventManager *eventManager) {

    if (eventManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    register_network_event_handler(eventManager, NE_CLIENT_CONNECT, handle_ne_client_connect_event);
    register_network_event_handler(eventManager, NE_CLIENT_DISCONNECT, handle_ne_client_disconnect_event);
    register_network_event_handler(eventManager, NE_CLIENT_MSG, handle_ne_client_msg_event);
    register_network_event_handler(eventManager, NE_ADD_POLL_FD, handle_ne_add_poll_fd_event);
    register_network_event_handler(eventManager, NE_REMOVE_POLL_FD, handle_ne_remove_poll_fd_event);
    register_system_event_handler(eventManager, SE_EXIT, handle_se_exit_event);
}