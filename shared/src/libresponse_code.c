#include "response_code.h"

#include <stddef.h>
#include <string.h>

struct ResponseCode {
    ResponseType responseType;
    int code;
    char *message;
};

static const ResponseCode RESPONSE_CODES[] = {
    {RPL_WELCOME, 1, "Welcome to the Internet Relay Network"},
    {ERR_NOSUCHCHANNEL, 403, "No such channel"},
    {ERR_UNKNOWNCOMMAND, 421, "Unknown command"},
    {ERR_NONICKNAMEGIVEN, 431, "No nickname given"},
    {ERR_ERRONEUSNICKNAME, 432, "Erroneous nickname"},
    {ERR_NICKNAMEINUSE, 433, "Nickname is already in use"},
    {ERR_NOTONCHANNEL, 442, "You're not on that channel"},
    {ERR_NOTREGISTERED, 451, "You have not registered"},
    {ERR_NEEDMOREPARAMS, 461, "Not enough parameters"},
    {ERR_ALREADYREGISTRED, 462, "Already registered"},
    {ERR_CHANNELISFULL, 471, "Cannot join channel"},
    {UNKNOWN_RESPONSE_TYPE, 0, "Unknown response type"},
};

_Static_assert(sizeof(RESPONSE_CODES) / sizeof(RESPONSE_CODES[0]) == RESPONSE_CODE_COUNT, "Array size mismatch");

int is_valid_code(int code) {

    int valid = 0;

    for (int i = 0; i < sizeof(RESPONSE_CODES)/ sizeof(RESPONSE_CODES[0]); i++) {

        if (RESPONSE_CODES[i].code == code) {
            valid = 1;
        }
    }

    return valid;
}

ResponseType get_response_type(int code) {

    ResponseType responseType = UNKNOWN_RESPONSE_TYPE;

    for (int i = 0; i < sizeof(RESPONSE_CODES)/ sizeof(RESPONSE_CODES[0]); i++) {

        if (RESPONSE_CODES[i].code == code) {
            responseType = RESPONSE_CODES[i].responseType;
        }
    }

    return responseType;
}

int get_response_code(ResponseType responseType) {

    int code = 0;

    for (int i = 0; i < sizeof(RESPONSE_CODES) / sizeof(RESPONSE_CODES[0]) && !code; i++) {

        if (RESPONSE_CODES[i].responseType == responseType) {
            code = RESPONSE_CODES[i].code;
        }
    }
    return code;
}

const char * get_response_message(int code) {

    const char *message = NULL;

    for (int i = 0; i < sizeof(RESPONSE_CODES)/ sizeof(RESPONSE_CODES[0]); i++) {

        if (RESPONSE_CODES[i].code == code) {
            message = RESPONSE_CODES[i].message;
        }
    }

    return message;
}
