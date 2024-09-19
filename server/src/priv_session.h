#ifndef H_SESSION
#define H_SESSION

#include "priv_user.h"
#include "priv_channel.h"
#include "../../shared/src/priv_hash_table.h"
#include "../../shared/src/priv_linked_list.h"

#define LOAD_FACTOR 2.0

typedef void (*IteratorFunction)(void *data, void *arg);

typedef struct {
    User *user;
    LinkedList *channels;
    int capacity;
    int count;
} UserChannels;

typedef struct {
    Channel *channel;
    LinkedList *users;
    int capacity;
    int count;
} ChannelUsers;

typedef struct {
    HashTable *users;
    HashTable *channels;
    LinkedList *userChannelsLL;
    LinkedList *channelUsersLL;
    int usersCapacity;
    int usersCount;
    int channelsCapacity;
    int channelsCount;
} Session;

Session * create_session(void);
void delete_session(Session *session);

void add_user_to_hash_table(Session *session, User *user);
void add_channel_to_hash_table(Session *session, Channel *channel);
void remove_user_from_hash_table(Session *session, User *user);
void remove_channel_from_hash_table(Session *session, Channel *channel);
User * find_user_in_hash_table(Session *session, const char *nickname);
Channel * find_channel_in_hash_table(Session *session, const char *name);
void change_user_in_hash_table(Session *session, User *oldUser, User *newUser);

UserChannels * create_user_channels(User *user);
ChannelUsers * create_channel_users(Channel *channel);

void add_user_channels(Session *session, UserChannels *userChannels);
void add_channel_users(Session *session, ChannelUsers *channelUsers);

int remove_user_channels(Session *session, UserChannels *userChannels);
int remove_channel_users(Session *session, ChannelUsers *channelUsers);

UserChannels * find_user_channels(Session *session, User *user);
ChannelUsers * find_channel_users(Session *session, Channel *channel);

int add_channel_to_user_channels(UserChannels *userChannels, Channel *channel);
int add_user_to_channel_users(ChannelUsers *channelUsers, User *user);

Channel * find_channel_in_user_channels(UserChannels *userChannels, Channel *channel);
User * find_user_in_channel_users(ChannelUsers *channelUsers, User *user);

void remove_channel_in_user_channels(UserChannels *userChannels, Channel *channel);
void remove_user_in_channel_users(ChannelUsers *channelUsers, User *user);

void change_user_in_user_channels(UserChannels *userChannels, User *user);
void change_user_in_channel_users(void *channelUsers, void *user);

void find_single_user_channels(void *channel, void *arg);

int are_user_channels_equal(void *userChannels1, void *userChannels2);
int are_channel_users_equal(void *channelUsers1, void *channelUsers2);

LinkedList * get_channels_from_user_channels(UserChannels *userChannels);
LinkedList * get_channel_users_ll(Session *session);
int get_users_count_from_channel_users(ChannelUsers *channelUsers);

#ifdef TEST

void delete_user_channels(void *userChannels);
void delete_channel_users(void *channelUsers);

#endif

#endif