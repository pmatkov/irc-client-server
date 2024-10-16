#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "tcp_server.h"
#include "../../libs/src/command.h"

typedef void (*CommandFunc)(TCPServer *tcpServer, Client *, CommandTokens *);

typedef struct ResponseTokens ResponseTokens;

void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType commandType);

#endif