/* --INTERNAL HEADER--
   used for testing */

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "priv_tcp_server.h"
#include "../../libs/src/priv_command.h"
#include "../../libs/src/priv_event.h"
#include "../../libs/src/response_code.h"

#ifdef MAX_CHARS
#undef MAX_CHARS
#define MAX_CHARS 510
#endif

typedef void (*CommandFunc)(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

void parse_message(const char *message, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType cmdType);

#ifdef TEST

void cmd_nick(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_user(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_join(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_part(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_privmsg(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_whois(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_quit(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

#endif

#endif