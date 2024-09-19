#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_USERS_PER_CHANNEL 100

typedef enum {
    PERMANENT,
    TEMPORARY,
    CHANNEL_TYPE_COUNT
} ChannelType;

typedef struct Channel Channel;

Channel * create_channel(const char *channelName, ChannelType channelType, int capacity);
void delete_channel(void *channel);

void add_message_to_channel(void *channel, void *content);

int are_channels_equal(void *channel1, void *channel2);

ChannelType get_channel_type(Channel *channel);
const char *get_channel_name(Channel *channel);

#endif