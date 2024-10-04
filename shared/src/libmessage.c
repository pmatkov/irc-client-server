#ifdef TEST
#include "priv_message.h"
#else
#include "message.h"
#endif

#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#define CRLF_LEN 2
#define LEAD_CHAR_LEN 1

#ifndef TEST

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

    safe_copy(regMessage->content, sizeof(regMessage->content), content);

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

    safe_copy(extMessage->sender, sizeof(extMessage->sender), sender);
    safe_copy(extMessage->recipient, sizeof(extMessage->recipient), recipient);
    safe_copy(extMessage->content, sizeof(extMessage->content), content); 

    return extMessage;
}

void delete_message(void *message) {

    free(message);
}

char get_char_from_message(void *message, int index, ContentRetrieveFunc contentRetrieveFunc) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char *content = contentRetrieveFunc(message);
    
    char ch = '\0';

    if (index >= 0 && index < strlen(content)) {
        ch = content[index];
    }

    return ch;
}

void set_char_in_message(void *message, char ch, int index, ContentRetrieveFunc contentRetrieveFunc) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char *content = contentRetrieveFunc(message);

    if (strlen(content) < MAX_CHARS - CRLF_LEN - LEAD_CHAR_LEN) {
        content[index] = ch;
    }
}

char * get_reg_message_content(void *message) {

    char *content = NULL;

    RegMessage *regMessage = message;

    if (regMessage != NULL) {

        content = regMessage->content;
    }
    
    return content;
}

char * get_ext_message_content(void *message) {

    char *content = NULL;

    ExtMessage *extMessage = message;

    if (extMessage != NULL) {

        content = extMessage->content;
    }
    
    return content;
}

char * get_ext_message_recipient(void *message) {

    char *recipient = NULL;

    ExtMessage *extMessage = message;

    if (extMessage != NULL) {

        recipient = extMessage->recipient;
    }
    
    return recipient;
}

