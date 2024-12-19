#ifndef IRC_MESSAGE_H
#define IRC_MESSAGE_H

#define MAX_TOKENS 5

typedef void (*MessagePrefixFunc)(char *buffer, int size, void *arg);

/*  IRC messages may have one of two forms. 
    a) if a message is a server response, the 
        format is the following:

        " <:prefix> <response code> <param 1> ... [param n] [:response message]"
        example: ":irc.server.com 431 * :No nickname given"

    b) if a message is forwarded by the server
        from one user to another or to several 
        users, the format is the following:

        " <:prefix> <message>"
        example: ":john!john@irc.client.com PRIVMSG #general :Hello" 

    IRC message consists of several tokens grouped
    into the prefix, the body and the suffix.
    the prefix may be included with the callback
    function */

typedef struct {
    const char *body[MAX_TOKENS];
    const char *suffix[MAX_TOKENS];
    int multiWordSuffix;
    MessagePrefixFunc messagePrefixFunc;
    void *funcArg;
} IRCMessage;

void create_irc_message(char *buffer, int size, IRCMessage *ircMessage);

#endif