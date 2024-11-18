#ifndef H_SESSION
#define H_SESSION

#include "user.h"
#include "channel.h"
#include "../../libs/src/linked_list.h"

#include <pthread.h>

typedef struct ReadyList ReadyList;

typedef struct UserChannels UserChannels;
typedef struct ChannelUsers ChannelUsers;
typedef struct Session Session;

typedef void (*IteratorFunc)(void *data, void *arg);

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
void register_new_channel_join(Session *session, Channel *channel, User *user);
void register_existing_channel_join(Session *session, Channel *channel, User *user);
void register_channel_leave(Session *session, Channel *channel, User *user);

void find_removable_channels(void *channel, void *arg);

int is_channel_full(ChannelUsers *channelUsers);

int are_user_channels_equal(void *userChannels1, void *userChannels2);
int are_channel_users_equal(void *channelUsers1, void *channelUsers2);

ReadyList * get_ready_list(Session *session);
LinkedList * get_ready_users(ReadyList *readyList);
LinkedList * get_ready_channels(ReadyList *readyList);

LinkedList * get_channels_from_user_channels(UserChannels *userChannels);
LinkedList * get_users_from_channel_users(ChannelUsers *channelUsers);

LinkedList * get_channel_users_ll(Session *session);
int get_channel_users_count(ChannelUsers *channelUsers);

#endif