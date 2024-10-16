#include "irc_message.h"

#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

/*  --IRC message format--
    - server response (when client sends an invalid command):
        " <:prefix> <response code> <param 1> ... [param n] [:response message]"
        example: ":irc.server.com 431 * :No nickname given"
    - forwarded message (when client sends a messsage to the user or channel): 
        " <:prefix> <message>"
        example: ":john!john@irc.client.com PRIVMSG #general :Hello"  */

void create_irc_message(char *buffer, int size, IRCMessageTokens *ircMesageTokens) {

    if (ircMesageTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char prefix[MAX_CHARS + 1] = {'\0'}; 
    int tkCount = concat_tokens(prefix, size, ircMesageTokens->prefix, ARR_SIZE(prefix), " ");

    if (tkCount) {
        prepend_char(prefix, MAX_CHARS, prefix, ':');
    }

    char body[MAX_CHARS + 1] = {'\0'}; 
    concat_tokens(body, size, ircMesageTokens->body, ARR_SIZE(body), " ");

    char suffix[MAX_CHARS + 1] = {'\0'}; 
    concat_tokens(suffix, size, ircMesageTokens->suffix, ARR_SIZE(suffix), " ");

    if (ircMesageTokens->useLeadChar) {
        prepend_char(suffix, MAX_CHARS, suffix, ':');
    }

    const char *tokens[] = {prefix, body, suffix};  

    concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), " ");
}


