/* --INTERNAL HEADER--
    used for testing */

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "priv_display.h"
#include "priv_tcp_client.h"
#include "priv_line_editor.h"
#include "../../libs/src/priv_command.h"

typedef void (*CommandFunc)(WindowManager *windowManager, TCPClient *tcpCLient, CommandTokens *cmdTokens);

void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens);

CommandFunc get_command_function(CommandType commandType);

#ifdef TEST

void cmd_connect(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_disconnect(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_nick(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_user(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_join(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_part(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_privmsg(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_address(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_port(WindowManager *windowManager, TCPClient *tcpClient, CommandTokens *cmdTokens);

#endif

#endif