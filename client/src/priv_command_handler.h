#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "priv_scrollback.h"
#include "priv_tcpclient.h"
#include "priv_line_editor.h"
#include "../../shared/src/priv_command.h"

typedef void (*CommandFunction)(Scrollback *, TCPClient *, CommandTokens *);

void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#ifdef TEST

void cmd_connect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);
void cmd_disconnect(Scrollback *scrollback, TCPClient *tcpClient, CommandTokens *cmdTokens);

#endif

#endif