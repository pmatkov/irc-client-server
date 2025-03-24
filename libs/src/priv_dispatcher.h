#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "priv_tcp_client.h"
#include "priv_display.h"
#include "../../libs/src/priv_poll_manager.h"
#include "../../libs/src/priv_io_utils.h"
#include "../../libs/src/priv_event.h"

void process_stdin_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpclient, CommandTokens *cmdTokens);
void process_pipe_data(EventManager *eventManager, StreamPipe *streamPipe);
void process_socket_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient);

void dispatch_events(EventManager *eventManager);

void handle_ui_key_event(Event *event);
void handle_ui_win_resize_event(Event *event);

void handle_ne_server_msg_event(Event *event);
void handle_ne_add_poll_fd_event(Event *event);
void handle_ne_remove_poll_fd_event(Event *event);
void handle_ne_peer_close_event(Event *event);

void handle_se_exit_event(Event *event);

void send_socket_messages(EventManager *eventManager, TCPClient *tcpClient);

void set_event_context(EventManager *eventManager, WindowManager *windowManager, PollManager *pollManager, TCPClient *tcpClient,  CommandTokens *cmdTokens);

void register_event_handlers(EventManager *eventManager);

#ifdef TEST

void execute_command(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void detect_pipe_event_type(const char *message, Event *event);

#endif

#endif