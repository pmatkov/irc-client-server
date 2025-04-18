#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_channel.h"
#else
#include "channel.h"
#include "../../libs/src/common.h"
#endif

#include "config.h"
#include "../../libs/src/common.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    pthread_rwlock_t channelLock;
    Channel *next;
};

#endif

static const char *CHANNEL_TYPE_STRINGS[] = {
    "Permanent",
    "Temporary",
    "Unknown"
};

ASSERT_ARRAY_SIZE(CHANNEL_TYPE_STRINGS, CHANNEL_TYPE_COUNT)

Channel * create_channel(const char *name, const char *topic, ChannelType channelType, int capacity) {

    Channel *channel = (Channel*) malloc(sizeof(Channel));
    if (channel == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    if (is_valid_channel_name(name)) {
        safe_copy(channel->name, sizeof(channel->name), name);
    }
    safe_copy(channel->topic, ARRAY_SIZE(channel->topic), topic);
    channel->channelType = channelType;
    channel->outQueue = create_queue(capacity, MAX_CHARS + 1);
    pthread_rwlock_init(&channel->channelLock, NULL);
    channel->next = NULL;

    return channel;
}

void delete_channel(void *channel) {

    if (channel != NULL) {
        delete_queue(((Channel*)channel)->outQueue);
        pthread_rwlock_destroy(&((Channel*)channel)->channelLock);
    }

    free(channel);
}

void enqueue_to_channel_queue(void *channel, void *content) {

    if (channel == NULL || content == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&((Channel *)channel)->channelLock);
    }
    enqueue(((Channel *)channel)->outQueue, content);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&((Channel *)channel)->channelLock);
    }
}

void * dequeue_from_channel_queue(Channel *channel) {

    if (channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&((Channel *)channel)->channelLock);
    }
    void *message = dequeue(((Channel *)channel)->outQueue);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&((Channel *)channel)->channelLock);
    }

    return message; 
}

bool are_channels_equal(void *channel1, void *channel2) {

    bool equal = 0;

    if (channel1 != NULL && channel2 != NULL) {
        equal = strcmp(((Channel*)channel1)->name, ((Channel*)channel2)->name) == 0;
    }
    return equal;

}

bool is_valid_channel_name(const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    bool valid = 0;

    if (string[0] == '#' && is_valid_name(string, "#-_\\[]{}|^~")) {
        valid = 1;
    };

    return valid;
}

const char *get_channel_name(Channel *channel) {

    if (channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return channel->name;
}

const char *get_channel_topic(Channel *channel) {

    if (channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return channel->topic;
}

ChannelType get_channel_type(Channel *channel) {

    if (channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return channel->channelType;
}

Queue * get_channel_queue(Channel *channel) {

    if (channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&((Channel *)channel)->channelLock);
    }
    Queue *queue = channel->outQueue;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&((Channel *)channel)->channelLock);
    }

    return queue;
}
