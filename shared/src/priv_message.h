#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_CHARS 512
#define MAX_NICKNAME_LEN 9
#define MAX_CHANNEL_LEN 50

typedef enum {
    REGULAR_MSG,
    EXTENDED_MSG,
    MESSAGE_TYPE_COUNT
} MessageType;

typedef struct {
    char content[MAX_CHARS + 1];
} RegMessage;

typedef struct {
    char sender[MAX_NICKNAME_LEN + 1];
    char recipient[MAX_CHANNEL_LEN + 1];
    char content[MAX_CHARS + 1];
} ExtMessage;

typedef char * (*GetMessageContent)(void *message);

RegMessage * create_reg_message(const char *content);
ExtMessage * create_ext_message(const char *sender, const char *recipient, const char *content);

void delete_message(void *message);

char get_char_from_message(void *message, int index, GetMessageContent getMessageContent);
void set_char_in_message(void *message, char ch, int index, GetMessageContent getMessageContent);

char * get_reg_message_content(void *message);
char * get_ext_message_content(void *message);

#endif