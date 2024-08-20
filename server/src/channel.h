#ifndef CHANNEL_H
#define CHANNEL_H

#include "../../shared/src/queue.h"
#include "user.h"

#define MAX_CHANNELS 20
#define MAX_USERS 20

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

typedef enum {
    PERSISTENT,
    TEMPORARY,
    UNKNOWN,
    CHTYPE_COUNT
} ChannelType;

typedef struct Channel {
    MessageQueue *messageQueue;
    char name[MAX_CHANNEL_LEN + 1];
    User **users;
    ChannelType channelType;
    int usersCount;
    struct Channel *next;  
} Channel;

typedef struct {
    Channel *head;
    int channelCount;
} ChannelList;

ChannelList * create_channel_list();
void delete_channel_list(ChannelList *channelList);

Channel * add_channel(ChannelList *channelList, char *name,     ChannelType channelType);
int remove_channel(ChannelList *channelList, char *name);
Channel *lookup_channel(ChannelList *channelList, char *name);

int add_user_to_channel(Channel *channel, User *user);
int remove_user_from_channel(Channel *channel, char *nickname);
User * lookup_user_in_channel(Channel *channel, char *nickname);

#ifdef TEST

STATIC Channel * create_channel(char *name, ChannelType channelType);
STATIC void delete_channel(Channel *channel);
STATIC int is_channel_list_empty(ChannelList *channelListl);
STATIC int is_channel_list_full(ChannelList *channelListl);
STATIC int is_valid_channel(ChannelType type);

STATIC int is_channel_empty(Channel *channel);
STATIC int is_channel_full(Channel *channel);

#endif

#endif
