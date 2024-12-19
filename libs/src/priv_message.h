/* --INTERNAL HEADER--
   used for testing */
#ifndef MESSAGE_H
#define MESSAGE_H

#include "common.h"

typedef enum {
    MSG_RESTART,
    MSG_SIGNAL,
    MSG_COMMAND,
    MSG_RESPONSE,
    MSG_STANDARD,
    MSG_PING,
    MSG_PONG,
    UNKNOWN_MESSAGE_TYPE,
    MESSAGE_TYPE_COUNT
} MessageType;

typedef enum {
    NO_PRIORITY,
    LOW_PRIORITY,
    NORMAL_PRIORTY,
    HIGH_PRIORITY,
    UNKNOWN_PRIORITY,
    MESSAGE_PRIORITY_COUNT
} MessagePriority;

typedef struct {
    char content[MAX_CHARS + 1];
    char separator[MAX_SEPARATOR_LEN + 1];
    MessageType messageType;
    MessagePriority messagePriority;
} Message;

Message * create_message(const char *content, const char *separator, MessageType messageType, MessagePriority messagePriority);
void delete_message(Message *message);

void set_message_char(Message *message, char ch, int index);

const char * get_message_content(Message *message);
void set_message_content(Message *message, const char *content);

MessageType get_message_type(Message *message);
void set_message_type(Message *message, MessageType type);
MessagePriority get_message_priority(Message *message);
void set_message_priority(Message *message, MessagePriority priority);

int get_message_size(void);

void serialize_message(char *buffer, int size, Message *message);
void deserialize_message(char *buffer, int size, Message *message);

#endif