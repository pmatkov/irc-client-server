#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "tcp_client.h"
#include "display.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/event.h"

/* processes data from standard input */
void process_stdin_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

/* processes data from a pipe */
void process_pipe_data(EventManager *eventManager, StreamPipe *streamPipe);

/* processes data from a socket */
void process_socket_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient);

void dispatch_events(EventManager *eventManager);
void handle_ui_key_event(Event *event);
void handle_ui_win_resize_event(Event *event);

void handle_ne_server_msg_event(Event *event);
void handle_ne_add_poll_fd_event(Event *event);
void handle_ne_remove_poll_fd_event(Event *event);
void handle_ne_peer_close_event(Event *event);

void handle_se_timer_event(Event *event);
void handle_se_exit_event(Event *event);

void send_socket_messages(EventManager *eventManager, TCPClient *tcpClient);

/* sets the event context for the dispatcher.
 *
 * This function initializes the event context with the provided window manager,
 * TCP client, and command tokens. It is used to configure the dispatcher to
 * handle events based on the given context.
 *
 */
void set_event_context(EventManager *eventManager, WindowManager *windowManager, PollManager *pollManager, TCPClient *tcpClient,  CommandTokens *cmdTokens);

/*
 * registers event handlers with the given event manager.
 *
 * This function is responsible for registering all necessary event handlers
 * with the provided EventManager instance. It ensures that the event manager
 * is aware of the handlers and can dispatch events to them appropriately.
 *
 */
void register_event_handlers(EventManager *eventManager);

#endif