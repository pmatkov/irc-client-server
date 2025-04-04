#define _XOPEN_SOURCE 700
/* --INTERNAL HEADER--
   used for testing */
   
#ifndef H_SESSION
#define H_SESSION

#include "priv_user.h"
#include "priv_channel.h"
#include "../../libs/src/priv_hash_table.h"
#include "../../libs/src/priv_linked_list.h"

#include <stdbool.h>
#include <pthread.h>

#define MAX_USERS 1024
#define MAX_CHANNELS 100

typedef void (*IteratorFunc)(void *data, void *arg);

typedef struct {
    LinkedList *readyUsers;
    LinkedList *readyChannels;
} ReadyList;

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
    ReadyList *readyList;
    HashTable *users;
    HashTable *channels;
    LinkedList *userChannelsLL;
    LinkedList *channelUsersLL;
    pthread_rwlock_t usersLock;
    pthread_rwlock_t channelsLock;
    pthread_rwlock_t usersCountLock;
    pthread_rwlock_t channelsCountLock;
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

ReadyList * create_ready_list(void);
void delete_ready_list(ReadyList *readyList);

void add_user_to_ready_list(void *user, void *readyList);
void add_channel_to_ready_list(void *channel, void *readyList);

void remove_user_from_ready_list(LinkedList *readyUsers, User *user);
void remove_channel_from_ready_list(LinkedList *readyChannels, Channel *channel);

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

void register_user(Session *session, User *user);
void unregister_user(Session *session, User *user);

void register_new_channel_join(Session *session, Channel *channel, User *user);
void register_existing_channel_join(Session *session, Channel *channel, User *user);

void register_channel_leave(Session *session, Channel *channel, User *user);
void leave_all_channels(Session *session, User *user, const char *message);
void remove_channel_data(Session *session, Channel *channel);

bool is_channel_full(ChannelUsers *channelUsers);

bool are_user_channels_equal(void *userChannels1, void *userChannels2);
bool are_channel_users_equal(void *channelUsers1, void *channelUsers2);

ReadyList * get_ready_list(Session *session);
LinkedList * get_ready_users(ReadyList *readyList);
LinkedList * get_ready_channels(ReadyList *readyList);

LinkedList * get_channels_from_user_channels(UserChannels *userChannels);
LinkedList * get_users_from_channel_users(ChannelUsers *channelUsers);

LinkedList * get_channel_users_ll(Session *session);
int get_channel_users_count(ChannelUsers *channelUsers);

#ifdef TEST

void delete_user_channels(void *userChannels);
void delete_channel_users(void *channelUsers);

#endif

#endif