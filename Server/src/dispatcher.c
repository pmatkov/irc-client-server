#include "dispatcher.h"
#include "main.h"

#include <stdlib.h>
#include <string.h>

STATIC int set_message(Message *message, char *sender, char *recipient, char *content);

MessageQueue * create_message_queue(int size) {

    MessageQueue *messageQueue = (MessageQueue*) malloc(sizeof(MessageQueue));
    if (messageQueue == NULL) {
        failed("Error allocating memory.");
    }

    messageQueue->messages = (Message*) calloc(size, sizeof(Message));
    if (messageQueue->messages == NULL) {
        failed("Error allocating memory.");
    }

    messageQueue->head = 0;
    messageQueue->tail = 0;
    messageQueue->allocatedSize = size;
    messageQueue->usedSize = 0; 

    return messageQueue;
}

void delete_message_queue(MessageQueue *messageQueue) {

    if (messageQueue != NULL) {
        free(messageQueue->messages);
        free(messageQueue);
    }
}

int is_empty(MessageQueue *messageQueue) {
    return messageQueue->usedSize == 0;
}

int is_full(MessageQueue *messageQueue) {
    return messageQueue->usedSize == messageQueue->allocatedSize;
}

STATIC int set_message(Message *message, char *sender, char *recipient, char *content) {

    if (strnlen(sender, MAX_NICKNAME_LEN + 1) == MAX_NICKNAME_LEN + 1\
        || strnlen(recipient, MAX_CHANNEL_LEN + 1) == MAX_CHANNEL_LEN + 1\
        || strnlen(content, MAX_MSG_LENGTH + 1) == MAX_MSG_LENGTH + 1) {
            return 0;
    }
    strcpy(message->sender, sender);
    strcpy(message->recipient, recipient);
    strcpy(message->content, content);

    return 1;
}

int enqueue(MessageQueue *messageQueue, char *sender, char *recipient, char *content) {
      
    if (!set_message(&messageQueue->messages[messageQueue->tail], sender, recipient, content)) {
        return 0;
    }

    if (is_full(messageQueue)) {
        messageQueue->head = (messageQueue->head + 1) % messageQueue->allocatedSize;
    }
    else {
        messageQueue->usedSize++;
    }

    messageQueue->tail = (messageQueue->tail + 1) % messageQueue->allocatedSize;

    return 1;
}

Message *dequeue(MessageQueue *messageQueue) {

    if (!is_empty(messageQueue)) {

        Message *message = &messageQueue->messages[messageQueue->head];

        messageQueue->head = (messageQueue->head + 1) % messageQueue->allocatedSize;
        messageQueue->usedSize--;

        return message;
    }
    else {
        return NULL;
    }

}