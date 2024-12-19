#ifndef RESPONSE_CODE_H
#define RESPONSE_CODE_H

/* represents IRC response codes */
typedef enum {
    RPL_WELCOME,
    RPL_WHOISUSER,
    RPL_NOTOPIC,
    RPL_TOPIC,
    RPL_NAMREPLY,
    RPL_ENDOFNAMES,
    ERR_NOSUCHNICK,
    ERR_NOSUCHCHANNEL,
    ERR_UNKNOWNCOMMAND,
    ERR_NONICKNAMEGIVEN,
    ERR_ERRONEUSNICKNAME,
    ERR_NICKNAMEINUSE,
    ERR_NOTONCHANNEL,
    ERR_NOTREGISTERED,
    ERR_NEEDMOREPARAMS,
    ERR_ALREADYREGISTRED,
    ERR_CHANNELISFULL,
    ERR_BADCHANNAME,
    UNKNOWN_RESPONSE_TYPE,
    RESPONSE_CODE_COUNT
} ResponseType;

/* represents a container for managing
    response codes */
typedef struct ResponseCode ResponseCode;

ResponseType get_response_type(const char *code);
const char * get_response_code(ResponseType responseType);
const char * get_response_message(const char *code);

#endif