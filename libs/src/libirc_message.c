#include "irc_message.h"

#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PREFIX 64

/*  --IRC message format--
    - server response (if client sends an invalid command):
        " <:prefix> <response code> <param 1> ... [param n] [:response message]"
        example: ":irc.server.com 431 * :No nickname given"
    - forwarded message (if client sends a messsage to the user or to the channel): 
        " <:prefix> <message>"
        example: ":john!john@irc.client.com PRIVMSG #general :Hello"  */

// void create_irc_message(char *buffer, int size, IRCMessageTokens *ircMessageTokens) {

//     if (buffer == NULL || ircMessageTokens == NULL) {
//         FAILED(ARG_ERROR, NULL);
//     }

//     char prefix[MAX_CHARS + 1] = {'\0'}; 
//     char body[MAX_CHARS + 1] = {'\0'}; 
//     char suffix[MAX_CHARS + 1] = {'\0'}; 

//     if (ircMessageTokens->prefix[0] != NULL) {
//         concat_tokens(prefix, ARR_SIZE(prefix), ircMessageTokens->prefix, ARR_SIZE(ircMessageTokens->prefix), " ");
//         prepend_char(prefix, MAX_CHARS, prefix, ':');
//     }

//     if (ircMessageTokens->body[0] != NULL) {
//         concat_tokens(body, ARR_SIZE(body), ircMessageTokens->body, ARR_SIZE(ircMessageTokens->body), " ");
//     }

//     if (ircMessageTokens->suffix[0] != NULL) {
//         concat_tokens(suffix, ARR_SIZE(suffix), ircMessageTokens->suffix, ARR_SIZE(ircMessageTokens->suffix), " ");
//     }

//     if (ircMessageTokens->multiWordSuffix) {
//         prepend_char(suffix, MAX_CHARS, suffix, ':');
//     }

//     const char *tokens[] = {prefix, body, suffix};

//     int spaceChars = ARR_SIZE(tokens) ? ARR_SIZE(tokens) - 1 : 0;
//     int messageLength = strlen(prefix) + strlen(body) + strlen(suffix) + spaceChars;

//     if (messageLength <= size) {
//         concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), " ");
//     }
//     else {
//         LOG(ERROR, "Max message length exceeded");
//     }
    
// }


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
        concat_tokens(body, ARR_SIZE(body), ircMessage->body, ARR_SIZE(ircMessage->body), " ");
    }

    if (ircMessage->suffix[0] != NULL) {
        concat_tokens(suffix, ARR_SIZE(suffix), ircMessage->suffix, ARR_SIZE(ircMessage->suffix), " ");
    }

    if (ircMessage->multiWordSuffix) {
        prepend_char(suffix, MAX_CHARS, suffix, ':');
    }

    const char *tokens[] = {prefix, body, suffix};

    int spaces = ARR_SIZE(tokens) ? ARR_SIZE(tokens) - 1 : 0;
    int messageLength = strlen(prefix) + strlen(body) + strlen(suffix) + spaces;

    if (messageLength <= size) {
        concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), " ");
    }
    else {
        LOG(ERROR, "Max message length exceeded");
    }
    
}



// void create_user_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname) {

//     if (buffer == NULL) {
//         FAILED(ARG_ERROR, NULL);
//     }

//     const char *tokens[] = {nickname, "!", username, "@", hostname};  
//     int userInfoLength = 0;

//     iterate_string_list(tokens, ARR_SIZE(tokens), add_string_length, &userInfoLength);

//     if (userInfoLength <= size) {
//         concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), " ");
//     }
//     else {
//         LOG(ERROR, "Max message length exceeded");
//     }
// }
