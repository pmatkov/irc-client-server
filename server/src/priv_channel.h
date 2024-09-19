#ifndef CHANNEL_H
#define CHANNEL_H

#include "../../shared/src/priv_queue.h"

#define MAX_CHANNEL_NAME 50
#define MAX_USERS_PER_CHANNEL 100

typedef enum {
    PERMANENT,
    TEMPORARY,
    CHANNEL_TYPE_COUNT
} ChannelType;

typedef struct Channel {
    char name[MAX_CHANNEL_NAME + 1];
    ChannelType channelType;
    Queue *outQueue;
    int capacity;
    int count;
    struct Channel *next;  
} Channel;

Channel * create_channel(const char *channelName, ChannelType channelType, int capacity);
void delete_channel(void *channel);

void add_message_to_channel(void *channel, void *content);

int are_channels_equal(void *channel1, void *channel2);

ChannelType get_channel_type(Channel *channel);
const char *get_channel_name(Channel *channel);

#endif