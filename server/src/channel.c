#ifdef TEST
#include "priv_channel.h"
#else
#include "channel.h"
#endif

#include "../../libs/src/priv_message.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_CHANNEL_NAME 50

#ifndef TEST

struct Channel {
    char name[MAX_CHANNEL_NAME + 1];
    char topic[MAX_CHARS + 1];
    ChannelType channelType;
    Queue *outQueue;
    Channel *next;
};

#endif

Channel * create_channel(const char *name, const char *topic, ChannelType channelType, int capacity) {

    Channel *channel = (Channel*) malloc(sizeof(Channel));
    if (channel == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    if (is_valid_name(name, 1)) {
        safe_copy(channel->name, MAX_CHANNEL_NAME + 1, name);
    }
    safe_copy(channel->topic, MAX_CHARS + 1, topic);
    channel->channelType = channelType;
    channel->outQueue = create_queue(capacity, sizeof(RegMessage));
    channel->next = NULL;

    return channel;
}

void delete_channel(void *channel) {

    if (channel != NULL) {
        delete_queue(((Channel*)channel)->outQueue);
    }

    free(channel);
}

void add_message_to_channel_queue(void *channel, void *content) {

    if (channel == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = create_reg_message(content);

    enqueue(((Channel *)channel)->outQueue, message);

    delete_message(message); 
}

void * remove_message_from_channel_queue(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = NULL;

    message = dequeue(((Channel *)channel)->outQueue);

    return message; 
}

int are_channels_equal(void *channel1, void *channel2) {

    int equal = 0;

    if (channel1 != NULL && channel2 != NULL) {
        equal = strcmp(((Channel*)channel1)->name, ((Channel*)channel2)->name) == 0;
    }
    return equal;

}

const char *get_channel_name(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channel->name;
}

const char *get_channel_topic(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channel->topic;
}

ChannelType get_channel_type(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channel->channelType;
}

Queue * get_channel_queue(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channel->outQueue;
}
