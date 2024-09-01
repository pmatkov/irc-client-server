#ifndef CHANNEL_H
#define CHANNEL_H

#include "../../shared/src/queue.h"
#include "test_user.h"

#define MAX_CHANNEL_NAME 50

typedef enum {
    PERSISTENT,
    TEMPORARY,
    CHANNEL_TYPE_COUNT
} ChannelType;

typedef struct Channel {
    MessageQueue *messageQueue;
    char name[MAX_CHANNEL_NAME + 1];
    User **users;
    ChannelType channelType;
    int usersCount;
    struct Channel *next;  
} Channel;

typedef struct {
    Channel *head;
    int channelCount;
} ChannelList;

ChannelList * create_channel_list(void);
void delete_channel_list(ChannelList *channelList);

Channel * add_channel(ChannelList *channelList, char *name,     ChannelType channelType);
int remove_channel(ChannelList *channelList, char *name);
Channel *lookup_channel(ChannelList *channelList, char *name);

int add_user_to_channel(Channel *channel, User *user);
int remove_user_from_channel(Channel *channel, char *nickname);
User * lookup_user_in_channel(Channel *channel, char *nickname);

#ifdef TEST

Channel * create_channel(char *name, ChannelType channelType);
void delete_channel(Channel *channel);

int is_channel_empty(Channel *channel);
int is_channel_full(Channel *channel);

int is_valid_channel_type(ChannelType type);
int is_channel_list_empty(ChannelList *channelListl);
int is_channel_list_full(ChannelList *channelListl);

#endif

#endif
