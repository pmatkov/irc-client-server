#ifdef TEST
#include "priv_channel.h"
#else
#include "channel.h"
#include "../../shared/src/queue.h"
#endif

#include "../../shared/src/priv_message.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

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
    ChannelType channelType;
    Queue *outQueue;
    int capacity;
    int count;
    Channel *next; 
};

#endif

Channel * create_channel(const char *channelName, ChannelType channelType, int capacity) {

    Channel *channel = (Channel*) malloc(sizeof(Channel));
    if (channel == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    channel->outQueue = create_queue(capacity, sizeof (RegMessage));

    if (is_valid_name(channelName, 1)) {

        safe_copy(channel->name, MAX_CHANNEL_NAME + 1, channelName);
    }

    channel->channelType = channelType;
    channel->next = NULL;

    return channel;
}

void delete_channel(void *channel) {

    if (channel != NULL) {
        delete_queue(((Channel*)channel)->outQueue);
    }

    free(channel);
}

void add_message_to_channel(void *channel, void *content) {

    if (channel == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = create_reg_message(content);

    enqueue(((Channel *)channel)->outQueue, message);

    delete_message(message); 
}

int are_channels_equal(void *channel1, void *channel2) {

    if (channel1 == NULL || channel2 == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return strcmp(((Channel*)channel1)->name, ((Channel*)channel2)->name) == 0;
}

ChannelType get_channel_type(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channel->channelType;
}

const char *get_channel_name(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channel->name;
}