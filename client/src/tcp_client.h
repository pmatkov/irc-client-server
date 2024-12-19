#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "../../libs/src/event.h"
#include "../../libs/src/queue.h"
#include "../../libs/src/session_state.h"
#include "../../libs/src/network_utils.h"

#include <stdbool.h>

/**
 * @brief The TCPClient provides networking functionality for the app.
 */
typedef struct TCPClient TCPClient;

/**
 * @brief Create a new TCPClient instance.
 * @return A pointer to the new TCPClient instance.
 */
TCPClient * create_client(void);

/**
 * @brief Delete a TCPClient instance.
 * @param tcpClient A pointer to the TCPClient instance to delete.
 */
void delete_client(TCPClient *tcpClient);

/**
 * @brief Connect to the server at the specified address and port.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param eventManager A pointer to the EventManager instance.
 * @param address The server address (hostname or IPv4).
 * @param port The server port.
 * @return 0 on success, -1 on error.
 */
int client_connect(TCPClient *tcpClient, EventManager *eventManager, const char *address, int port);

/**
 * @brief Close the connection to the server.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param eventManager A pointer to the EventManager instance.
 */
void client_disconnect(TCPClient *tcpClient, EventManager *eventManager);

/**
 * @brief Terminates the session for the given TCP client.
 *
 * This function closes the connection and performs any necessary cleanup
 * for the specified TCP client.
 *
 * @param tcpClient A pointer to the TCPClient structure representing the client session to be terminated.
 */
void terminate_session(TCPClient *tcpClient);

/**
 * @brief Read data from the socket.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param eventManager A pointer to the EventManager instance.
 * @return -1 on server disconnect and error, 1 if full message was received, 0 otherwise.
 */
int client_read(TCPClient *tcpClient, EventManager *eventManager);

/**
 * @brief Write data to the socket.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param eventManager A pointer to the EventManager instance.
 * @param message The message to write.
 */
void client_write(TCPClient *tcpClient, EventManager *eventManager, const char *message);

/**
 * @brief Add a message to the outbound queue.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param message The message to add to the queue.
 */
void enqueue_to_client_queue(TCPClient *tcpClient, void *message);

/**
 * @brief Remove a message from the outbound queue.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return The message removed from the queue.
 */
void * dequeue_from_client_queue(TCPClient *tcpClient);

/**
 * @brief Get the file descriptor of the client.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return The file descriptor of the client.
 */
int get_client_fd(TCPClient *tcpClient);

/**
 * @brief Set the file descriptor of the client.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param fd The file descriptor to set.
 */
void set_client_fd(TCPClient *tcpClient, int fd);

/**
 * @brief Get the server identifier.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return The server identifier.
 */
const char * get_server_identifier(TCPClient *tcpClient);

/**
 * @brief Set the server identifier.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param identifier The string to set as the server identifier
 * @param identifierType The type of server identifier
 * @return The server identifier.
 */
void set_server_identifier(TCPClient *tcpClient, const char *serverIdentifier, HostIdentifierType identifierType);

/**
 * @brief Get the client's input buffer.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return The client's input buffer.
 */
char * get_client_inbuffer(TCPClient *tcpClient);

/**
 * @brief Set the client's input buffer.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param string The string to set as the input buffer.
 */
void set_client_inbuffer(TCPClient *tcpClient, const char *string);

/**
 * @brief Get the client's outbound queue.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return A pointer to the client's outbound queue.
 */
Queue * get_client_queue(TCPClient *tcpClient);

/**
 * @brief Get the client's state type.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return The client's state type.
 */
SessionStateType get_client_state_type(TCPClient *tcpClient);

/**
 * @brief Set the client's state.
 * @param tcpClient A pointer to the TCPClient instance.
 * @param clientState The client's state.
 */
void set_client_state_type(TCPClient *tcpClient, SessionStateType stateType);

/**
 * @brief Check if the client is connected.
 * @param tcpClient A pointer to the TCPClient instance.
 * @return True if the client is connected, false otherwise.
 */
bool is_client_connected(TCPClient *tcpClient);


#endif