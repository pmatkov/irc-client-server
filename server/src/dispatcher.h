#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "tcp_server.h"
#include "../../libs/src/event.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/command.h"
#include "../../libs/src/io_utils.h"

void process_connection_request(EventManager *eventManager, TCPServer *tcpServer);
void process_pipe_data(EventManager *eventManager, StreamPipe *streamPipe);
void process_socket_data(EventManager *eventManager, TCPServer *tcpServer, int fd);

void dispatch_events(EventManager *eventManager);

void handle_ne_client_connect_event(Event *event);
void handle_ne_client_disconnect_event(Event *event);
void handle_ne_add_poll_fd_event(Event *event);
void handle_ne_remove_poll_fd_event(Event *event);
void handle_ne_client_msg_event(Event *event);
void handle_se_exit_event(Event *event);

void send_socket_messages(EventManager *eventManager, TCPServer *tcpServer);

void set_event_context(EventManager *eventManager, PollManager *pollManager, TCPServer *tcpServer, CommandTokens *cmdTokens);

void register_event_handlers(EventManager *eventManager);

#endif