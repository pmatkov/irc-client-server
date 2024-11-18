#ifndef IRC_MESSAGE_H
#define IRC_MESSAGE_H

#define MAX_TOKENS 5

typedef void (*MessagePrefixFunc)(char *buffer, int size, void *arg);

/* each IRC message is divided into three parts:
    the prefix, the body and the suffix. each of
    these parts may consist of several tokens */
// typedef struct {
//     const char *prefix[MAX_TOKENS];
//     const char *body[MAX_TOKENS];
//     const char *suffix[MAX_TOKENS];
//     int multiWordSuffix;
// } IRCMessageTokens;

typedef struct {
    const char *body[MAX_TOKENS];
    const char *suffix[MAX_TOKENS];
    int multiWordSuffix;
    MessagePrefixFunc messagePrefixFunc;
    void *funcArg;
} IRCMessage;


/* construct IRC message from the tokens and 
    save it to the buffer */
// void create_irc_message(char *buffer, int size, IRCMessageTokens *ircMessageTokens);

void create_irc_message(char *buffer, int size, IRCMessage *ircMessage);

#endif