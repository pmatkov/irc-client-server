#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "scrollback.h"
#include "tcpclient.h"
#include "line_editor.h"
#include "../../libs/src/command.h"

typedef void (*CommandFunction)(Scrollback *, TCPClient *, CommandTokens *);

/* parses command line input. if the parsed 
    input is a valid command the specified
    command will be executed. */
void parse_input(LineEditor *lnEditor, CommandTokens *cmdTokens);

CommandFunction get_command_function(CommandType commandType);

#endif