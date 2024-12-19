#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "tcp_client.h"
#include "display.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/event.h"

/**
 * @brief Processes data from standard input.
 * 
 * @param eventManager Pointer to the EventManager.
 * @param windowManager Pointer to the WindowManager.
 * @param tcpClient Pointer to the TCPClient.
 * @param cmdTokens Pointer to the CommandTokens.
 */
void process_stdin_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

/**
 * @brief Processes data from a pipe.
 * 
 * @param eventManager Pointer to the EventManager.
 * @param streamPipe Pointer to the StreamPipe.
 */
void process_pipe_data(EventManager *eventManager, StreamPipe *streamPipe);

/**
 * @brief Processes data from a socket.
 * 
 * @param eventManager Pointer to the EventManager.
 * @param windowManager Pointer to the WindowManager.
 * @param tcpClient Pointer to the TCPClient.
 */
void process_socket_data(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient);

/**
 * @brief Dispatches events to the appropriate handlers.
 * 
 * @param eventManager Pointer to the EventManager.
 */
void dispatch_events(EventManager *eventManager);

/**
 * @brief Handles a key event from the UI.
 * 
 * @param event Pointer to the Event.
 */
void handle_ui_key_event(Event *event);

/**
 * @brief Handles a window resize event from the UI.
 * 
 * @param event Pointer to the Event.
 */
void handle_ui_win_resize_event(Event *event);

/**
 * @brief Handles a server message event from the network.
 * 
 * @param event Pointer to the Event.
 */
void handle_ne_server_msg_event(Event *event);

/**
 * @brief Handles the event of adding a poll file descriptor.
 *
 *
 * @param event Pointer to the Event structure containing the details of the event.
 */
void handle_ne_add_poll_fd_event(Event *event);

/**
 * @brief Handles the event of removing a poll file descriptor.
 *
 * @param event Pointer to the Event structure containing the details of the event.
 */
void handle_ne_remove_poll_fd_event(Event *event);

/**
 * @brief Handles a peer close event from the network.
 * 
 * @param event Pointer to the Event.
 */
void handle_ne_peer_close_event(Event *event);

/**
 * @brief Handles the timer event.
 *
 * @param event Pointer to the Event.
 */
void handle_se_timer_event(Event *event);

/**
 * @brief Handles an exit event from the system.
 * 
 * @param event Pointer to the Event.
 */
void handle_se_exit_event(Event *event);

/**
 * @brief Sends messages through the socket.
 * 
 * @param eventManager Pointer to the EventManager.
 * @param tcpClient Pointer to the TCPClient.
 */
void send_socket_messages(EventManager *eventManager, TCPClient *tcpClient);

/**
 * @brief Sets the event context for the dispatcher.
 *
 * This function initializes the event context with the provided window manager,
 * TCP client, and command tokens. It is used to configure the dispatcher to
 * handle events based on the given context.
 *
 * @param eventManager Pointer to the EventManager.
 * @param windowManager Pointer to the WindowManager.
 * @param pollManager Pointer to the PollManager.
 * @param tcpClient Pointer to the TCPClient.
 * @param cmdTokens Pointer to the CommandTokens.
 */
void set_event_context(EventManager *eventManager, WindowManager *windowManager, PollManager *pollManager, TCPClient *tcpClient,  CommandTokens *cmdTokens);

/**
 * @brief Registers event handlers with the given event manager.
 *
 * This function is responsible for registering all necessary event handlers
 * with the provided EventManager instance. It ensures that the event manager
 * is aware of the handlers and can dispatch events to them appropriately.
 *
 * @param eventManager Pointer to the EventManager instance where the event
 *                     handlers will be registered.
 */
void register_event_handlers(EventManager *eventManager);

#endif