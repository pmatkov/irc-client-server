#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "scrollback.h"
#include "tcpclient.h"
#include "line_editor.h"
#include "../../shared/src/command.h"

typedef void (*CommandFunction)(Scrollback *, TCPClient *, CommandTokens *);

void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#endif