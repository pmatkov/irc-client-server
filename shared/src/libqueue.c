#ifdef TEST
#include "test_queue.h"
#else
#include "queue.h"
#endif

#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct MessageQueue {
    void *messages;
    DataType dataType; 
    size_t itemSize;
    int head;
    int tail;
    int searchIndex;
    int allocatedSize;
    int usedSize;
};

#endif

STATIC int is_valid_data_type(DataType dataType);
STATIC size_t get_type_size(DataType dataType);

STATIC int is_valid_data_type(DataType dataType) {

    return dataType >= 0 && dataType < DATA_TYPE_COUNT;
}

STATIC size_t get_type_size(DataType dataType) {

    if (!is_valid_data_type(dataType)) {
        FAILED("Invalid data type", NO_ERRCODE);
    }
    
    if (dataType == REGULAR_MSG) {
        return sizeof(RegMessage);
    }
    else if (dataType == EXTENDED_MSG) {
        return sizeof(ExtMessage);
    }

    return 0;
}

MessageQueue * create_message_queue(DataType dataType, int allocationSize) {

    if (!is_valid_data_type(dataType)) {
        FAILED("Invalid data type", NO_ERRCODE);
    }

    MessageQueue *messageQueue = (MessageQueue*) malloc(sizeof(MessageQueue));
    if (messageQueue == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    messageQueue->messages = (void*) calloc(allocationSize, get_type_size(dataType));
    if (messageQueue->messages == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    messageQueue->dataType = dataType;
    messageQueue->itemSize = get_type_size(dataType);
    messageQueue->head = 0;
    messageQueue->tail = 0;
    messageQueue->searchIndex = 0;
    messageQueue->allocatedSize = allocationSize;
    messageQueue->usedSize = 0; 

    return messageQueue;
}

void delete_message_queue(MessageQueue *messageQueue) {

    if (messageQueue != NULL) {
        free(messageQueue->messages);
    }
    free(messageQueue); 
}

int mq_is_empty(MessageQueue *messageQueue) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return messageQueue->usedSize == 0;
}

int mq_is_full(MessageQueue *messageQueue) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return messageQueue->usedSize == messageQueue->allocatedSize;
}

void set_char_in_message(void *message, char ch, int index) {
    
    if (strlen(((RegMessage*)message)->content) < MAX_MSG) {

        ((RegMessage*)message)->content[index] = ch;
    }
}

char get_char_from_message(void *message, int index) {
    
    char ch = '\0';

    if (index < strlen(((RegMessage*)message)->content)) {
        ch = ((RegMessage*)message)->content[index];
    }

    return ch;
}

int set_reg_message(void *message, const char *content) {

    if (message == NULL || content == NULL) {
        FAILED("Invalid data type", ARG_ERROR);
    }

    if (strnlen(content, MAX_MSG + 1) == MAX_MSG + 1) {
        return 0;
    }

    strcpy(((RegMessage*)message)->content, content);

    return 1;
}

int set_ext_message(void *message, const char *sender, const char *recipient, const char *content) {

    if (message == NULL || sender == NULL || recipient == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strnlen(sender, MAX_NICKNAME + 1) == MAX_NICKNAME + 1\
        || strnlen(recipient, MAX_CHANNEL + 1) == MAX_CHANNEL + 1\
        || strnlen(content, MAX_MSG + 1) == MAX_MSG + 1) {
            return 0;
    }
    strcpy(((ExtMessage*)message)->sender, sender);
    strcpy(((ExtMessage*)message)->recipient, recipient);
    strcpy(((ExtMessage*)message)->content, content);

    return 1;
}

char *get_message_content(void *message) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return ((RegMessage*)message)->content;
}

void enqueue(MessageQueue *messageQueue, void *message) {

    if (messageQueue == NULL || message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    unsigned char *target = (unsigned char *)messageQueue->messages + messageQueue->tail * messageQueue->itemSize;

    memcpy(target, message, messageQueue->itemSize);

    if (mq_is_full(messageQueue)) {
        messageQueue->head = (messageQueue->head + 1) % messageQueue->allocatedSize;

    }
    else {
        messageQueue->usedSize++;
    }

    messageQueue->tail = (messageQueue->tail + 1) % messageQueue->allocatedSize;
    messageQueue->searchIndex = messageQueue->tail;

}

void *dequeue(MessageQueue *messageQueue) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (mq_is_empty(messageQueue)) {
        return NULL;
    }

    unsigned char *message = (unsigned char *)messageQueue->messages + messageQueue->head * messageQueue->itemSize;

    messageQueue->head = (messageQueue->head + 1) % messageQueue->allocatedSize;
    messageQueue->usedSize--;

    return message;

}

void *get_message(MessageQueue *messageQueue, int direction) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    unsigned char *message;

    if (direction < 0 && messageQueue->searchIndex == messageQueue->tail) {
        message = NULL;
    }
    else if (direction > 0 && messageQueue->searchIndex == messageQueue->head) {
        message = NULL;
    }
    else {
        if (direction < 0) {
            messageQueue->searchIndex = (messageQueue->searchIndex + 1) % messageQueue->allocatedSize;
        }
        else if (direction > 0) {
            messageQueue->searchIndex = (messageQueue->searchIndex - 1) % messageQueue->allocatedSize;
        }
        message = (unsigned char *)messageQueue->messages + messageQueue->searchIndex * messageQueue->itemSize;
    }

    return message;

}