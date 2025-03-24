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

/* function pointer type for command functions */
typedef void (*CommandFunc)(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

/* parses and validates user input commands */
void parse_cli_input(InputWindow *inputWindow, CommandTokens *cmdTokens);

/* retrieves the function pointer for a given command type */
CommandFunc get_command_function(CommandType cmdType);

#endif