#ifndef PARSER_H
#define PARSER_H

#include "tcpserver.h"
#include "../../shared/src/priv_command.h"

typedef void (*CommandFunction)(TCPServer *, Client *, CommandTokens *);

void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#ifdef TEST

void create_fwd_message(char *buffer, int size, const char *prefix, const char *body, const char *suffix);
void create_response(char *buffer, int size, const char *prefix, const char *body, ResponseType responseType);
void create_client_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname);

#endif

#endif