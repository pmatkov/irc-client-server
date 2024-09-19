#ifdef TEST
#include "priv_message.h"
#else
#include "message.h"
#endif

#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#ifndef TEST

#define MAX_CHARS 512
#define MAX_NICKNAME_LEN 9
#define MAX_CHANNEL_LEN 50

struct RegMessage {
    char content[MAX_CHARS + 1];
};

struct ExtMessage {
    char sender[MAX_NICKNAME_LEN + 1];
    char recipient[MAX_CHANNEL_LEN + 1];
    char content[MAX_CHARS + 1];
};

#endif

RegMessage * create_reg_message(const char *content) {

    if (content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *regMessage = (RegMessage*) calloc(1, sizeof(RegMessage));
    if (regMessage == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    safe_copy(regMessage->content, MAX_CHARS + 1, content);

    return regMessage;
}

ExtMessage * create_ext_message(const char *sender, const char *recipient, const char *content) {

    if (sender == NULL || recipient == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    ExtMessage *extMessage = (ExtMessage*) calloc(1, sizeof(ExtMessage));
    if (extMessage == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    safe_copy(extMessage->sender, MAX_NICKNAME_LEN + 1, sender);
    safe_copy(extMessage->recipient, MAX_CHANNEL_LEN + 1, recipient);
    safe_copy(extMessage->content, MAX_CHARS + 1, content); 

    return extMessage;
}

void delete_message(void *message) {

    free(message);
}

char get_char_from_message(void *message, int index, GetMessageContent getMessageContent) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char *content = getMessageContent(message);
    
    char ch = '\0';

    if (index >= 0 && index < strlen(content)) {
        ch = content[index];
    }

    return ch;
}

void set_char_in_message(void *message, char ch, int index, GetMessageContent getMessageContent) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char *content = getMessageContent(message);

    if (strlen(content) < MAX_CHARS) {
        content[index] = ch;
    }
}

char * get_reg_message_content(void *message) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    RegMessage *regMessage = (RegMessage *)message;
    return regMessage->content;
}

char * get_ext_message_content(void *message) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    ExtMessage *extMessage = (ExtMessage *)message;
    return extMessage->content;
}
