#ifndef MESSAGE_H
#define MESSAGE_H

#include "string_utils.h"

typedef enum {
    REGULAR_MSG,
    EXTENDED_MSG,
    MESSAGE_TYPE_COUNT
} MessageType;

typedef char * (*ContentRetrieveFunc)(void *message);

typedef struct RegMessage RegMessage;
typedef struct ExtMessage ExtMessage;

RegMessage * create_reg_message(const char *content);
ExtMessage * create_ext_message(const char *sender, const char *recipient, const char *content);

void delete_message(void *message);

char get_char_from_message(void *message, int index, ContentRetrieveFunc contentRetrieveFunc);
void set_char_in_message(void *message, char ch, int index, ContentRetrieveFunc contentRetrieveFunc);

char * get_reg_message_content(void *message);
char * get_ext_message_content(void *message);

char * get_ext_message_recipient(void *message);

#endif