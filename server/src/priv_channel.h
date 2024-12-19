/* --INTERNAL HEADER--
   used for testing */
#define _XOPEN_SOURCE 700

#ifndef CHANNEL_H
#define CHANNEL_H

#include "../../libs/src/priv_queue.h"
#include "../../libs/src/common.h"

#include <stdbool.h>
#include <pthread.h>

#define MAX_CHANNEL_LEN 50
#define MAX_USERS_PER_CHANNEL 100

typedef enum {
    PERMANENT,
    TEMPORARY,
    UNKNOWN_CHANNEL_TYPE,
    CHANNEL_TYPE_COUNT
} ChannelType;

typedef struct Channel {
    char name[MAX_CHANNEL_LEN + 1];
    char topic[MAX_CHARS + 1];
    ChannelType channelType;
    Queue *outQueue;
    pthread_rwlock_t channelLock;
    struct Channel *next;  
} Channel;

Channel * create_channel(const char *name, const char *topic, ChannelType channelType, int capacity);
void delete_channel(void *channel);

void enqueue_to_channel_queue(void *channel, void *content);
void * dequeue_from_channel_queue(Channel *channel);

bool are_channels_equal(void *channel1, void *channel2);
bool is_valid_channel_name(const char *string);

const char *get_channel_name(Channel *channel);
const char *get_channel_topic(Channel *channel);
ChannelType get_channel_type(Channel *channel);
Queue * get_channel_queue(Channel *channel);

#endif