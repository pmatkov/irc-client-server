#include "irc_message.h"

#include "common.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PREFIX 64

void create_irc_message(char *buffer, int size, IRCMessage *ircMessage) {

    if (buffer == NULL || ircMessage == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char prefix[MAX_PREFIX + 1] = {'\0'}; 
    char body[MAX_CHARS + 1] = {'\0'}; 
    char suffix[MAX_CHARS + 1] = {'\0'}; 

    if (ircMessage->messagePrefixFunc != NULL && ircMessage->funcArg != NULL) {

        ircMessage->messagePrefixFunc(prefix, MAX_PREFIX, ircMessage->funcArg);
    }

    if (strlen(prefix)) {
        prepend_char(prefix, MAX_CHARS, prefix, ':');
    }

    if (ircMessage->body[0] != NULL) {
        concat_tokens(body, ARRAY_SIZE(body), ircMessage->body, ARRAY_SIZE(ircMessage->body), " ");
    }

    if (ircMessage->suffix[0] != NULL) {
        concat_tokens(suffix, ARRAY_SIZE(suffix), ircMessage->suffix, ARRAY_SIZE(ircMessage->suffix), " ");
    }

    if (ircMessage->multiWordSuffix) {
        prepend_char(suffix, MAX_CHARS, suffix, ':');
    }

    const char *tokens[] = {prefix, body, suffix};

    int spaces = ARRAY_SIZE(tokens) ? ARRAY_SIZE(tokens) - 1 : 0;
    int messageLength = strlen(prefix) + strlen(body) + strlen(suffix) + spaces;

    if (messageLength <= size) {
        concat_tokens(buffer, size, tokens, ARRAY_SIZE(tokens), " ");
    }
    else {
        LOG(ERROR, "Max message length exceeded");
    }
    
}
