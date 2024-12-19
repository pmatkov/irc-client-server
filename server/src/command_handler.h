#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "tcp_server.h"
#include "../../libs/src/event.h"
#include "../../libs/src/command.h"

#ifdef MAX_CHARS
#undef MAX_CHARS
#define MAX_CHARS 510
#endif

typedef void (*CommandFunc)(EventManager *eventManager, TCPServer *tcpServer, Client *, CommandTokens *);

void parse_message(const char *message, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType cmdType);

#endif