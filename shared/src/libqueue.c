#include "queue.h"
#include "errorctrl.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

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

    messageQueue->messages = (void*) malloc(allocationSize * get_type_size(dataType));
    if (messageQueue->messages == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    messageQueue->dataType = dataType;
    messageQueue->itemSize = get_type_size(dataType);
    messageQueue->head = 0;
    messageQueue->tail = 0;
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

int is_empty(MessageQueue *messageQueue) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return messageQueue->usedSize == 0;
}

int is_full(MessageQueue *messageQueue) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return messageQueue->usedSize == messageQueue->allocatedSize;
}

int set_reg_message(void *message, char *content) {

    if (message == NULL || content == NULL) {
        FAILED("Invalid data type", ARG_ERROR);
    }

    if (strnlen(content, MAX_MSG_LEN + 1) == MAX_MSG_LEN + 1) {
        return 0;
    }

    strcpy(((RegMessage*)message)->content, content);

    return 1;
}

int set_ext_message(void *message, char *sender, char *recipient, char *content) {

    if (message == NULL || sender == NULL || recipient == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strnlen(sender, MAX_NICKNAME_LEN + 1) == MAX_NICKNAME_LEN + 1\
        || strnlen(recipient, MAX_CHANNEL_LEN + 1) == MAX_CHANNEL_LEN + 1\
        || strnlen(content, MAX_MSG_LEN + 1) == MAX_MSG_LEN + 1) {
            return 0;
    }
    strcpy(((ExtMessage*)message)->sender, sender);
    strcpy(((ExtMessage*)message)->recipient, recipient);
    strcpy(((ExtMessage*)message)->content, content);

    return 1;
}

int enqueue(MessageQueue *messageQueue, void *message) {

    if (messageQueue == NULL || message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    unsigned char *target = (unsigned char *)messageQueue->messages + messageQueue->tail * messageQueue->itemSize;

    memcpy(target, message, messageQueue->itemSize);

    if (is_full(messageQueue)) {
        messageQueue->head = (messageQueue->head + 1) % messageQueue->allocatedSize;
    }
    else {
        messageQueue->usedSize++;
    }

    messageQueue->tail = (messageQueue->tail + 1) % messageQueue->allocatedSize;

    return 1;
}

void *dequeue(MessageQueue *messageQueue) {

    if (messageQueue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_empty(messageQueue)) {
        return NULL;
    }

    unsigned char *message = (unsigned char *)messageQueue->messages + messageQueue->head * messageQueue->itemSize;

    messageQueue->head = (messageQueue->head + 1) % messageQueue->allocatedSize;
    messageQueue->usedSize--;

    return message;

}