#ifndef MESSAGE_H
#define MESSAGE_H

typedef enum {
    REGULAR_MSG,
    EXTENDED_MSG,
    MESSAGE_TYPE_COUNT
} MessageType;

typedef char * (*GetMessageContent)(void *message);

typedef struct RegMessage RegMessage;
typedef struct ExtMessage ExtMessage;

RegMessage * create_reg_message(const char *content);
ExtMessage * create_ext_message(const char *sender, const char *recipient, const char *content);

void delete_message(void *message);

char get_char_from_message(void *message, int index, GetMessageContent getMessageContent);
void set_char_in_message(void *message, char ch, int index, GetMessageContent getMessageContent);

char * get_reg_message_content(void *message);
char * get_ext_message_content(void *message);

#endif