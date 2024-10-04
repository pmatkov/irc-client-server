#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "priv_tcpserver.h"
#include "../../shared/src/priv_command.h"
#include "../../shared/src/response_code.h"

typedef void (*CommandFunction)(TCPServer *tcpServer, Client *, CommandTokens *);

typedef struct {
    const char *prefix[MAX_TOKENS];
    const char *body[MAX_TOKENS];
    const char *suffix[MAX_TOKENS];
    int useSuffixPrefix;
} ResponseTokens;

void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#ifdef TEST

void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

void create_client_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname);

#endif

#endif