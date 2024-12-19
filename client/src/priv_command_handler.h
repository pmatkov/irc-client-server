/* --INTERNAL HEADER--
    used for testing */
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "priv_display.h"
#include "priv_tcp_client.h"
#include "priv_input_window.h"
#include "../../libs/src/priv_event.h"
#include "../../libs/src/priv_command.h"

#ifdef MAX_CHARS
#undef MAX_CHARS
#define MAX_CHARS 510
#endif

typedef void (*CommandFunc)(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

void parse_cli_input(InputWindow *inputWindow, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType commandType);

#ifdef TEST

void parse_string(const char *string, CommandTokens *cmdTokens);
void set_uppercase_command(char *result, int size, const char *string);
void set_connection_params(CommandTokens *cmdTokens, char *ipv4address, int *port);
void register_connection(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void set_nickname(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void set_user_data(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

void cmd_connect(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_disconnect(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_nick(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_user(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_join(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_part(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_privmsg(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_address(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_port(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_whois(EventManager *eventManager, WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

#endif

#endif