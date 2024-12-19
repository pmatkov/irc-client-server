#ifdef TEST
#include "priv_message.h"
#else
#include "message.h"
#include "common.h"
#endif

#include "enum_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>

static const char *MESSAGE_TYPE_STRINGS[] = {
    "Relay",
    "Signal",
    "Command",
    "Response",
    "Standard",
    "Ping",
    "Pong",
    "Unknown"
};

ASSERT_ARRAY_SIZE(MESSAGE_TYPE_STRINGS, MESSAGE_TYPE_COUNT)

Message * create_message(const char *content, const char *separator, MessageType messageType, MessagePriority messagePriority) {

    if (content == NULL || separator == NULL || !is_valid_enum_type(messageType, MESSAGE_TYPE_COUNT) || \
        !is_valid_enum_type(messagePriority, MESSAGE_PRIORITY_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }

    Message *message = (Message*) malloc(sizeof(Message));
    if (message == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    safe_copy(message->content, ARRAY_SIZE(message->content), content);
    safe_copy(message->separator, ARRAY_SIZE(message->separator), separator);
    message->messageType = messageType;
    message->messagePriority = messagePriority;

    return message;
}

void delete_message(Message *message) {

    free(message);
}

void set_message_char(Message *message, char ch, int index) {

    if (message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (index >= 0 && index < ARRAY_SIZE(message->content)) {
        message->content[index] = ch;
    } 
}

const char * get_message_content(Message *message) {

    if (message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return message->content;
}

void set_message_content(Message *message, const char *content) {

    if (message == NULL || content == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    safe_copy(message->content, ARRAY_SIZE(message->content), content);
}

MessageType get_message_type(Message *message) {

    if (message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return message->messageType;
}

void set_message_type(Message *message, MessageType type) {

    if (message == NULL || !is_valid_enum_type(type, MESSAGE_TYPE_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }
    message->messageType = type;
}

MessagePriority get_message_priority(Message *message) {

    if (message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return message->messagePriority;
}

void set_message_priority(Message *message, MessagePriority priority) {

    if (message == NULL || !is_valid_enum_type(priority, MESSAGE_PRIORITY_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }
    message->messagePriority = priority;
}

int get_message_size(void) {

    return sizeof(Message);
}

void serialize_message(char *buffer, int size, Message *message) {

    if (buffer == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (ARRAY_SIZE(message->content) + sizeof(message->separator) + 2 * sizeof(int) > size) {
        LOG(ERROR, "Message size exceeds buffer size");
        return;
    }

    int messageType = htonl(message->messageType);
    int messagePriority = htonl(message->messagePriority);
    safe_copy(buffer, ARRAY_SIZE(message->content), message->content);
    memcpy(buffer + ARRAY_SIZE(message->content), message->separator, ARRAY_SIZE(message->separator));
    memcpy(buffer + ARRAY_SIZE(message->content) + ARRAY_SIZE(message->separator), &messageType, sizeof(messageType));
    memcpy(buffer + ARRAY_SIZE(message->content) + ARRAY_SIZE(message->separator) + sizeof(messageType), &messagePriority, sizeof(messagePriority));
}

void deserialize_message(char *buffer, int size, Message *message) {

    if (buffer == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int expected_size = ARRAY_SIZE(message->content) + sizeof(message->separator) + 2 * sizeof(int);
    if (size < expected_size) {
        LOG(ERROR, "Buffer size is smaller than expected for deserialization");
        return;
    }

    safe_copy(message->content, ARRAY_SIZE(message->content), buffer);
    memcpy(message->separator, buffer + ARRAY_SIZE(message->content), ARRAY_SIZE(message->separator));

    int messageType;
    memcpy(&messageType, buffer + ARRAY_SIZE(message->content) + ARRAY_SIZE(message->separator), sizeof(messageType));
    message->messageType = ntohl(messageType);

    int messagePriority;
    memcpy(&messagePriority, buffer + ARRAY_SIZE(message->content) + ARRAY_SIZE(message->separator) + sizeof(messageType), sizeof(messagePriority));
    message->messagePriority = ntohl(messagePriority);
}
