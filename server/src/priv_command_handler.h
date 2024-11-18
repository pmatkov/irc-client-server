/* --INTERNAL HEADER--
   used for testing */

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "priv_tcp_server.h"
#include "../../libs/src/priv_command.h"
#include "../../libs/src/response_code.h"

#ifdef MAX_CHARS
#undef MAX_CHARS
#define MAX_CHARS 510
#endif

#define CRLF_LEN 2

typedef void (*CommandFunc)(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

typedef struct {
    const char *prefix[MAX_TOKENS];
    const char *body[MAX_TOKENS];
    const char *suffix[MAX_TOKENS];
    int useSuffixPrefix;
} ResponseTokens;

void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType commandType);

#ifdef TEST

void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

#endif

#endif