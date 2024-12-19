#include "response_code.h"
#include "common.h"
#include "string_utils.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>

/* a response code is identified by its response type,
    numeric code and response message */
struct ResponseCode {
    ResponseType responseType;
    const char *code;
    const char *message;
};

/* a list of response codes identifiers, numeric codes and 
    response messages based on the IRC standard */
static const ResponseCode RESPONSE_CODES[] = {
    {RPL_WELCOME, "001", "Welcome to the IRC Network"},
    {RPL_WHOISUSER, "311", ""},
    {RPL_NOTOPIC, "331", "No topic is set"},
    {RPL_TOPIC, "332", ""},
    {RPL_NAMREPLY, "353", ""},
    {RPL_ENDOFNAMES, "366", "End of /NAMES list"},
    {ERR_NOSUCHNICK, "401", "No such nick"},
    {ERR_NOSUCHCHANNEL, "403", "No such channel"},
    {ERR_UNKNOWNCOMMAND, "421", "Unknown command"},
    {ERR_NONICKNAMEGIVEN, "431", "No nickname given"},
    {ERR_ERRONEUSNICKNAME, "432", "Erroneous nickname"},
    {ERR_NICKNAMEINUSE, "433", "Nickname is already in use"},
    {ERR_NOTONCHANNEL, "442", "You're not on that channel"},
    {ERR_NOTREGISTERED, "451", "You have not registered"},
    {ERR_NEEDMOREPARAMS, "461", "Not enough parameters"},
    {ERR_ALREADYREGISTRED, "462", "Already registered"},
    {ERR_CHANNELISFULL, "471", "Cannot join channel"},
    {ERR_BADCHANNAME, "479", "Illegal channel name"},
    {UNKNOWN_RESPONSE_TYPE, "0", "Unknown response type"},
};

ASSERT_ARRAY_SIZE(RESPONSE_CODES, RESPONSE_CODE_COUNT)

// static_assert(ARRAY_SIZE(RESPONSE_CODES) == RESPONSE_CODE_COUNT, "Array size mismatch");

ResponseType get_response_type(const char *code) {

    ResponseType responseType = UNKNOWN_RESPONSE_TYPE;

    for (int i = 0; i < sizeof(RESPONSE_CODES)/ sizeof(RESPONSE_CODES[0]); i++) {

        if (strcmp(RESPONSE_CODES[i].code, code) == 0) {
            responseType = RESPONSE_CODES[i].responseType;
        }
    }

    return responseType;
}

const char * get_response_code(ResponseType responseType) {

    const char *code = NULL;

    for (int i = 0; i < sizeof(RESPONSE_CODES) / sizeof(RESPONSE_CODES[0]); i++) {

        if (RESPONSE_CODES[i].responseType == responseType) {
            code = RESPONSE_CODES[i].code;
        }
    }
    return code;
}

const char * get_response_message(const char *code) {

    const char *message = NULL;

    for (int i = 0; i < sizeof(RESPONSE_CODES)/ sizeof(RESPONSE_CODES[0]); i++) {

        if (strcmp(RESPONSE_CODES[i].code, code) == 0) {
            message = RESPONSE_CODES[i].message;
        }
    }

    return message;
}
