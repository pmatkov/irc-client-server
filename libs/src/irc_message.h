#ifndef IRC_MESSAGE_H
#define IRC_MESSAGE_H

#define MAX_TOKENS 5

/* each IRC message is divided into three 
    parts: the prefix, the body and the 
    suffix. each of these parts may consist
    of several tokens. IRC messages may be sent
    by the client or the server */
typedef struct {
    const char *prefix[MAX_TOKENS];
    const char *body[MAX_TOKENS];
    const char *suffix[MAX_TOKENS];
    int useLeadChar;
} IRCMessageTokens;

/* construct IRC message from the tokens and 
    save to buffer */
void create_irc_message(char *buffer, int size, IRCMessageTokens *ircMesageTokens);

#endif