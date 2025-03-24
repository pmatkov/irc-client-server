#ifdef TEST
#include "priv_dispatcher.h"
#else
#include "dispatcher.h"
#endif

#include "config.h"
#include "input_controller.h"
#include "command_handler.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
#include "../../libs/src/enum_utils.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/common.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

typedef struct {
    EventManager *eventManager;
    WindowManager *windowManager;
    PollManager *pollManager;
    TCPClient *tcpClient;
    CommandTokens *cmdTokens;
} EventContext;

static EventContext eventContext = {NULL};

STATIC void execute_command(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
STATIC void detect_pipe_event_type(const char *message, Event *event);

void process_stdin_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (eventManager == NULL || windowManager == NULL || tcpClient == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *inputBaseWindow = get_le_base_window(get_input_window(windowManager));

    int ch = get_char(inputBaseWindow);
    ch = remap_ctrl_key(ch);

    Event event = {.eventType = UI_EVENT, .subEventType = UI_KEY, .dataItem = (DataItem) {.itemInt = ch}, .dataType = INT_TYPE};

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

        Event event = {.eventType = UNKNOWN_EVENT_TYPE, .subEventType = -1, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE};

        detect_pipe_event_type(message, &event);

        if (event.eventType != UNKNOWN_EVENT_TYPE) {
            push_event_to_queue(eventManager, &event);
        }
    }

    if (is_pipe_buffer_full(streamPipe)) {
        reset_pipe_buffer(streamPipe);
    }
}

void process_socket_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient) {

    if (eventManager == NULL || windowManager == NULL ||  tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int readStatus = client_read(tcpClient, eventManager);

    if (readStatus == -1) {

        push_event_to_queue(eventManager, &(Event){.eventType = NETWORK_EVENT, .subEventType = NE_PEER_CLOSE, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE});

    }
    else if (readStatus == 1) {

        char message[MAX_CHARS + 1] = {'\0'};

        while (extract_message(message, ARRAY_SIZE(message), get_client_inbuffer(tcpClient), CRLF)) {

            Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_SERVER_MSG, .dataItem = (DataItem) {.itemChar = ""}, .dataType = CHAR_TYPE};
            safe_copy(event.dataItem.itemChar, ARRAY_SIZE(event.dataItem.itemChar), message);
  
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

        if (get_event_type(event) == UI_EVENT) {
            dispatch_ui_event(eventManager, event);
        }
        if (get_event_type(event) == NETWORK_EVENT) {
            dispatch_network_event(eventManager, event);
        }
        if (get_event_type(event) == SYSTEM_EVENT) {
            dispatch_system_event(eventManager, event);
        }
    }
}

void handle_ui_key_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    KeyboardCmdFunc keyCmdFunc = get_keyboard_cmd_function(event->dataItem.itemInt);

    BaseWindow *mainBaseWindow = get_active_window(get_main_windows(eventContext.windowManager));
    BaseWindow *inputBaseWindow = get_le_base_window(get_input_window(eventContext.windowManager));

    if (keyCmdFunc != NULL) {

        WindowType windowType = code_to_window_type(event->dataItem.itemInt);

        if (windowType == SCROLLBACK_WINDOW) {
            keyCmdFunc(mainBaseWindow);
        }
        else if (windowType == INPUT_WINDOW) {
            keyCmdFunc(inputBaseWindow);
        }
    } 
    else if (isprint(event->dataItem.itemInt)) {
        add_char(inputBaseWindow, event->dataItem.itemInt);
    }
    else if (event->dataItem.itemInt == KEY_NEWLINE) {
        parse_cli_input(get_input_window(eventContext.windowManager), eventContext.cmdTokens);
        execute_command(eventContext.eventManager, eventContext.windowManager, eventContext.tcpClient, eventContext.cmdTokens);
    } 
    wrefresh(get_window(inputBaseWindow));
}

void handle_ui_win_resize_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    resize_ui(eventContext.windowManager, get_int_option_value(OT_COLOR));
}

void handle_ne_server_msg_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    display_server_message(event->dataItem.itemChar, eventContext.windowManager);
}

void handle_ne_add_poll_fd_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    set_poll_fd(eventContext.pollManager, event->dataItem.itemInt);
}

void handle_ne_remove_poll_fd_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unset_poll_fd(eventContext.pollManager, event->dataItem.itemInt);
    close(event->dataItem.itemInt);
}

void handle_ne_peer_close_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *statusWindow = get_status_window(eventContext.windowManager);
    InputWindow *inputWindow = get_input_window(eventContext.windowManager);

    display_response(eventContext.windowManager, "Server terminated.");
    display_status(statusWindow, inputWindow, &(StatusParams){MAIN_STATUS, 0, 0, 0, {"", ""}, "", 0});
}

void handle_ne_client_disconnect_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    send_socket_messages(eventContext.eventManager, eventContext.tcpClient);
    terminate_session(eventContext.tcpClient);
}

void handle_se_timer_event(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    display_time_status(eventContext.windowManager);
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
    else if (strcmp(message, "sigalrm") == 0) {
        event->eventType = SYSTEM_EVENT;
        event->subEventType = SE_TIMER;
    } 
    else if (strcmp(message, "sigwinch") == 0) {
        event->eventType = UI_EVENT;
        event->subEventType = UI_WIN_RESIZE;
    } 
}

STATIC void execute_command(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (eventManager == NULL || windowManager == NULL || tcpClient == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *command = get_command(cmdTokens);

    if (command != NULL) {
            
        CommandType cmdType = string_to_command_type(command);

        CommandFunc commandFunc = get_command_function(cmdType);
        commandFunc(eventManager, windowManager, tcpClient, cmdTokens);

        reset_command_tokens(cmdTokens);
    }
}

void send_socket_messages(EventManager *eventManager, TCPClient *tcpClient) {

    if (eventManager == NULL || tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    while (!is_queue_empty(get_client_queue(tcpClient))) {

        char *content = dequeue_from_client_queue(tcpClient);

        client_write(tcpClient, eventManager, content);
    } 
}

void set_event_context(EventManager *eventManager, WindowManager *windowManager, PollManager *pollManager, TCPClient *tcpClient, CommandTokens *cmdTokens) {

    if (eventManager == NULL || windowManager == NULL || pollManager == NULL || tcpClient == NULL  || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    eventContext.eventManager = eventManager;
    eventContext.windowManager = windowManager;
    eventContext.pollManager = pollManager;
    eventContext.tcpClient = tcpClient;
    eventContext.cmdTokens = cmdTokens;
}

void register_event_handlers(EventManager *eventManager) {

    if (eventManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    register_ui_event_handler(eventManager, UI_KEY, handle_ui_key_event);
    register_ui_event_handler(eventManager, UI_WIN_RESIZE, handle_ui_win_resize_event);
    register_network_event_handler(eventManager, NE_SERVER_MSG, handle_ne_server_msg_event);
    register_network_event_handler(eventManager, NE_ADD_POLL_FD, handle_ne_add_poll_fd_event);
    register_network_event_handler(eventManager, NE_REMOVE_POLL_FD, handle_ne_remove_poll_fd_event);
    register_network_event_handler(eventManager, NE_PEER_CLOSE, handle_ne_peer_close_event);
    register_network_event_handler(eventManager, NE_CLIENT_DISCONNECT, handle_ne_client_disconnect_event);
    register_system_event_handler(eventManager, SE_TIMER, handle_se_timer_event);
    register_system_event_handler(eventManager, SE_EXIT, handle_se_exit_event);
}