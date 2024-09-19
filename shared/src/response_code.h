#ifndef RESPONSE_CODE_H
#define RESPONSE_CODE_H

typedef enum {
    RPL_WELCOME,
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
    UNKNOWN_RESPONSE_TYPE,
    RESPONSE_CODE_COUNT
} ResponseType;

typedef struct ResponseCode ResponseCode;

ResponseType get_response_type(int code);
int get_response_code(ResponseType responseType);
const char * get_response_message(int code);

#endif