#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "scrollback.h"
#include "settings.h"
#include "tcpclient.h"
#include "line_editor.h"
#include "../../shared/src/priv_command.h"

typedef void (*CommandFunction)(Scrollback *, Settings *, TCPClient *, CommandTokens *);

void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#ifdef TEST

void create_notice(char *buffer, int size, const char **tokens, int tkCount);

#endif

#endif