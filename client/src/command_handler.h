#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "display.h"
#include "tcp_client.h"
#include "input_window.h"
#include "../../libs/src/event.h"
#include "../../libs/src/command.h"

#ifdef MAX_CHARS
#undef MAX_CHARS
#define MAX_CHARS 510
#endif

/**
 * @typedef CommandFunc
 * @brief Function pointer type for command functions.
 * 
 * @param eventManager Pointer to the EventManager instance.
 * @param windowManager Pointer to the WindowManager instance.
 * @param tcpClient Pointer to the TCPClient instance.
 * @param cmdTokens Pointer to the CommandTokens instance.
 */
typedef void (*CommandFunc)(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

/**
 * @brief Parses and validates user input commands.
 * 
 * The user controls the client app with keyboard commands. Each command is parsed and validated.
 * Valid commands are executed and saved for future access via command history.
 * 
 * @param inputWindow Pointer to the InputWindow instance.
 * @param cmdTokens Pointer to the CommandTokens instance.
 */
void parse_cli_input(InputWindow *inputWindow, CommandTokens *cmdTokens);

/**
 * @brief Retrieves the function pointer for a given command type.
 * 
 * @param cmdType The type of the command.
 * @return CommandFunc The function pointer associated with the command type.
 */
CommandFunc get_command_function(CommandType cmdType);

#endif