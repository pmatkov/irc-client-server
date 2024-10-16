/* --INTERNAL HEADER--
   used for testing */

#ifndef CHANNEL_H
#define CHANNEL_H

#include "../../libs/src/priv_queue.h"
#include "../../libs/src/string_utils.h"

#define MAX_CHANNEL_NAME 50
#define MAX_USERS_PER_CHANNEL 100

typedef enum {
    PERMANENT,
    TEMPORARY,
    CHANNEL_TYPE_COUNT
} ChannelType;

typedef struct Channel {
    char name[MAX_CHANNEL_NAME + 1];
    char topic[MAX_CHARS + 1];
    ChannelType channelType;
    Queue *outQueue;
    struct Channel *next;  
} Channel;

Channel * create_channel(const char *name, const char *topic, ChannelType channelType, int capacity);
void delete_channel(void *channel);

void add_message_to_channel_queue(void *channel, void *content);
void * remove_message_from_channel_queue(Channel *channel);

int are_channels_equal(void *channel1, void *channel2);

const char *get_channel_name(Channel *channel);
const char *get_channel_topic(Channel *channel);
ChannelType get_channel_type(Channel *channel);
Queue * get_channel_queue(Channel *channel);

#endif