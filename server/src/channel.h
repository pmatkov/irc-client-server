#ifndef CHANNEL_H
#define CHANNEL_H

#include "../../shared/src/queue.h"
#include "user.h"

typedef enum {
    PERSISTENT,
    TEMPORARY,
    CHANNEL_TYPE_COUNT
} ChannelType;

typedef struct Channel Channel;
typedef struct ChannelList ChannelList;

ChannelList * create_channel_list(void);
void delete_channel_list(ChannelList *channelList);

Channel * add_channel(ChannelList *channelList, char *name,     ChannelType channelType);
int remove_channel(ChannelList *channelList, char *name);
Channel *lookup_channel(ChannelList *channelList, char *name);

int add_user_to_channel(Channel *channel, User *user);
int remove_user_from_channel(Channel *channel, char *nickname);
User * lookup_user_in_channel(Channel *channel, char *nickname);

#endif
