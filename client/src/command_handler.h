#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "display.h"
#include "tcp_client.h"
#include "line_editor.h"
#include "../../libs/src/command.h"

typedef void (*CommandFunc)(WindowManager *windowManager, TCPClient *tcpCLient, CommandTokens *cmdTokens);

/* parse command line input */
void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType commandType);

#endif