#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "tcpserver.h"
#include "../../shared/src/command.h"

typedef void (*CommandFunction)(TCPServer *, Client *, CommandTokens *);

void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#endif