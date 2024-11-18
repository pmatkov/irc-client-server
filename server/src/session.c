#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_session.h"
#else
#include "session.h"
#include "../../libs/src/hash_table.h"
#endif

#include "main.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

#define START_USERS 500
#define START_CHANNELS 50

#define MAX_USERS START_USERS * LOAD_FACTOR
#define MAX_CHANNELS START_CHANNELS * LOAD_FACTOR

#define LOAD_FACTOR 2.0

/* keeps track of all user's channels */
struct UserChannels {
    User *user;
    LinkedList *channels;
    int capacity;
    int count;
};

/* keeps track of all users in a channel */
struct ChannelUsers {
    Channel *channel;
    LinkedList *users;
    int capacity;
    int count;
};

/* keeps track of users and channnels
  which have messages to send */ 
struct ReadyList {
    LinkedList *readyUsers;
    LinkedList *readyChannels;
};
 
/* keeps track of all users on the server, 
all channels on the server, all user's 
channels and all users in a channel */
struct Session {
    ReadyList *readyList;
    HashTable *users;
    HashTable *channels;
    LinkedList *userChannelsLL;
    LinkedList *channelUsersLL;
    int usersCapacity;
    int usersCount;
    int channelsCapacity;
    int channelsCount;
    pthread_rwlock_t usersLock;
    pthread_rwlock_t channelsLock;
    pthread_rwlock_t usersCountLock;
    pthread_rwlock_t channelsCountLock;
};

#endif

static pthread_rwlock_t readyListLock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_rwlock_t userChannelsLock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_rwlock_t channelUsersLock = PTHREAD_RWLOCK_INITIALIZER;

STATIC void delete_user_channels(void *userChannels);
STATIC void delete_channel_users(void *channelUsers);

Session * create_session(void) {

    Session *session = (Session *) malloc(sizeof(Session));
    if (session == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }
    session->readyList = create_ready_list();

    session->users = create_hash_table(START_USERS, string_hash_function, are_strings_equal, NULL, delete_user);
    session->channels = create_hash_table(START_CHANNELS, string_hash_function, are_strings_equal, NULL, delete_channel);

    session->userChannelsLL = create_linked_list(are_user_channels_equal, delete_user_channels);
    session->channelUsersLL = create_linked_list(are_channel_users_equal, delete_channel_users);

    session->usersCapacity = MAX_USERS;
    session->usersCount = 0;
    session->channelsCapacity = MAX_CHANNELS;
    session->channelsCount = 0;

    pthread_rwlock_init(&session->usersLock, NULL);
    pthread_rwlock_init(&session->channelsLock, NULL);
    pthread_rwlock_init(&session->usersCountLock, NULL);
    pthread_rwlock_init(&session->channelsCountLock, NULL);

    return session; 
}

void delete_session(Session *session) {

    if (session != NULL) {

        delete_ready_list(session->readyList);
        delete_hash_table(session->users);
        delete_hash_table(session->channels);
        delete_linked_list(session->userChannelsLL);
        delete_linked_list(session->channelUsersLL);

        pthread_rwlock_destroy(&session->usersLock);
        pthread_rwlock_destroy(&session->channelsLock);
        pthread_rwlock_destroy(&session->usersCountLock);
        pthread_rwlock_destroy(&session->channelsCountLock);

    }

    free(session);    
}

/* don't manually free user or channel after 
adding them to the <hash table>. they will be
freed automatically when hash table is deleted
in delete_session() if DeleteValueFunc is passed
to create_hash_table() */
void add_user_to_hash_table(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&session->usersLock);
        pthread_rwlock_wrlock(&session->usersCountLock);
    }

    if (session->usersCount < session->usersCapacity && find_item_in_hash_table(session->users, (char*)get_user_nickname(user)) == NULL) {

        HashItem *item = create_hash_item((char*)get_user_nickname(user), user); 

        int inserted = insert_item_to_hash_table(session->users, item);

        if (inserted) {
            session->usersCount++;
        }
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&session->usersCountLock);
        pthread_rwlock_unlock(&session->usersLock);
    }

}

void add_channel_to_hash_table(Session *session, Channel *channel) {

    if (session == NULL || channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&session->channelsLock);
        pthread_rwlock_wrlock(&session->channelsCountLock);
    }

    if (session->channelsCount < session->channelsCapacity && find_item_in_hash_table(session->channels, (char*) get_channel_name(channel)) == NULL) {

        HashItem *item = create_hash_item((char*)get_channel_name(channel), channel); 

        int inserted = insert_item_to_hash_table(session->channels, item);

        if (inserted) {
            session->channelsCount++;
        }
    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&session->channelsCountLock);
        pthread_rwlock_unlock(&session->channelsLock);
    }
} 

void remove_user_from_hash_table(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&session->usersLock);
        pthread_rwlock_wrlock(&session->usersCountLock);
    }

    if (session->usersCount) {

        int removed = remove_item_from_hash_table(session->users, (char*)get_user_nickname(user));

        if (removed) {
            session->usersCount--;
        }
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&session->usersCountLock);
        pthread_rwlock_unlock(&session->usersLock);
    }
}

void remove_channel_from_hash_table(Session *session, Channel *channel) {

    if (session == NULL || channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&session->channelsLock);
        pthread_rwlock_wrlock(&session->channelsCountLock);
    }

    if (session->channelsCount) {

        int removed = remove_item_from_hash_table(session->channels, (char*)get_channel_name(channel));

        if (removed) {
            session->channelsCount--;
        }
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&session->channelsCountLock);
        pthread_rwlock_unlock(&session->channelsLock);
    }
}

User * find_user_in_hash_table(Session *session, const char *nickname) {

    if (session == NULL || nickname == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&session->usersLock);
    }

    HashItem *item = find_item_in_hash_table(session->users, (char*)nickname);
    User *user = get_value(item);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&session->usersLock);
    }

    return user;
}

Channel * find_channel_in_hash_table(Session *session, const char *name) {

    if (session == NULL || name == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&session->channelsLock);
    }

    HashItem *item = find_item_in_hash_table(session->channels, (char*)name);
    Channel *channel = get_value(item);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&session->channelsLock);
    }

    return channel;
}

void change_user_in_hash_table(Session *session, User *oldUser, User *newUser) {

    if (session == NULL || oldUser == NULL || newUser == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    remove_user_from_hash_table(session, oldUser);
    add_user_to_hash_table(session, newUser);
}

ReadyList * create_ready_list(void) {
    
    ReadyList *readyList = (ReadyList *) malloc(sizeof(ReadyList));
    if (readyList == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    readyList->readyUsers = create_linked_list(are_users_equal, NULL);
    readyList->readyChannels = create_linked_list(are_channels_equal, NULL);

    return readyList;
}

void delete_ready_list(ReadyList *readyList) {

    if (readyList != NULL) {

        delete_linked_list(readyList->readyUsers);
        delete_linked_list(readyList->readyChannels);
    }

    free(readyList);   
}

void add_user_to_ready_list(void *user, void *readyList) {

    if (user == NULL || readyList == NULL ) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&readyListLock);
    }

    if (!find_node(((ReadyList*)readyList)->readyUsers, user)) {

        Node *node = create_node(user);
        append_node(((ReadyList*)readyList)->readyUsers, node);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&readyListLock);
    }
}

void add_channel_to_ready_list(void *channel, void *readyList) {

    if (channel == NULL || readyList == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&readyListLock);
    }

    if (!find_node(((ReadyList*)readyList)->readyChannels, channel)) {

        Node *node = create_node(channel);
        append_node(((ReadyList*)readyList)->readyChannels, node);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&readyListLock);
    }
}

UserChannels * create_user_channels(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    UserChannels *userChannels = (UserChannels*) malloc(sizeof(UserChannels));
    if (userChannels == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    userChannels->user = user;
    userChannels->channels = create_linked_list(are_channels_equal, NULL);
    userChannels->capacity = MAX_CHANNELS_PER_USER;
    userChannels->count = 0;

    return userChannels;
}

ChannelUsers * create_channel_users(Channel *channel) {

    if (channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    ChannelUsers *channelUsers = (ChannelUsers*) malloc(sizeof(ChannelUsers));
    if (channelUsers == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    channelUsers->channel = channel;
    channelUsers->users = create_linked_list(are_users_equal, NULL);
    channelUsers->capacity = MAX_USERS_PER_CHANNEL;
    channelUsers->count = 0;

    return channelUsers;
}

STATIC void delete_user_channels(void *userChannels) {

    if (userChannels != NULL) {

        delete_linked_list(((UserChannels*)userChannels)->channels);
    }

    free(userChannels);
}

STATIC void delete_channel_users(void *channelUsers) {

    if (channelUsers != NULL) {

        delete_linked_list(((ChannelUsers*)channelUsers)->users);
    }

    free(channelUsers);
}

/* don't manually free userChannels user or 
channelUsers after adding them to <linked list>, 
they will be freed automatically when linked list 
is deleted in delete_session() if DeleteDataFunc 
is passed to create_linked_list() */

void add_user_channels(Session *session, UserChannels *userChannels) {

    if (session == NULL || userChannels == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&userChannelsLock);
    }

    Node *node = create_node(userChannels);
    append_node(session->userChannelsLL, node);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

}

void add_channel_users(Session *session, ChannelUsers *channelUsers) {

    if (session == NULL || channelUsers == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&channelUsersLock);
    }

    Node *node = create_node(channelUsers);
    append_node(session->channelUsersLL, node);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }
}

int remove_user_channels(Session *session, UserChannels *userChannels) {

    if (session == NULL || userChannels == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&userChannelsLock);
    }

    int removed = remove_node(session->userChannelsLL, userChannels);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

    return removed;
}

int remove_channel_users(Session *session, ChannelUsers *channelUsers) {

    if (session == NULL || channelUsers == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&channelUsersLock);
    }

    int removed = remove_node(session->channelUsersLL, channelUsers);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return removed;
}

UserChannels * find_user_channels(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&userChannelsLock);
    }

    Node *node = find_node(session->userChannelsLL, &(UserChannels){user, NULL, 0, 0});

    UserChannels *userChannels = get_data(node);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

    return userChannels;
}

ChannelUsers * find_channel_users(Session *session, Channel *channel) {

    if (session == NULL || channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }

    Node *node = find_node(session->channelUsersLL, &(ChannelUsers){channel, NULL, 0, 0});

    ChannelUsers *channelUsers = get_data(node);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return channelUsers;
}

int add_channel_to_user_channels(UserChannels *userChannels, Channel *channel) {

    if (userChannels == NULL || channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int added = 0;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&userChannelsLock);
    }

    if (userChannels->count < userChannels->capacity) {

        Node *node = create_node(channel);
        append_node(userChannels->channels, node);
        userChannels->count++;
        added = 1;
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

    return added;
}

int add_user_to_channel_users(ChannelUsers *channelUsers, User *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int added = 0;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&channelUsersLock);
    }

    if (channelUsers->count < channelUsers->capacity) {

        Node *node = create_node(user);
        append_node(channelUsers->users, node);
        channelUsers->count++;
        added = 1;
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return added;
}

Channel * find_channel_in_user_channels(UserChannels *userChannels, Channel *channel) {

    if (userChannels == NULL || channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&userChannelsLock);
    }

    Node *node = find_node(userChannels->channels, channel);
    Channel *foundChannel = get_data(node);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

    return foundChannel;
}

User * find_user_in_channel_users(ChannelUsers *channelUsers, User *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }

    Node *node = find_node(channelUsers->users, user);
    User *foundUser = get_data(node);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return foundUser;
}

void remove_channel_in_user_channels(UserChannels *userChannels, Channel *channel) {

    if (userChannels == NULL || channel == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&userChannelsLock);
    }

    remove_node(userChannels->channels, channel);
    userChannels->count--;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }
}

void remove_user_in_channel_users(ChannelUsers *channelUsers, User *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&channelUsersLock);
    }

    remove_node(channelUsers->users, user);
    channelUsers->count--;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }
}

void change_user_in_user_channels(UserChannels *userChannels, User *user) {

    if (userChannels == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&userChannelsLock);
    }

    userChannels->user = user;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }
}

void change_user_in_channel_users(void *channelUsers, void *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&channelUsersLock);
    }

    LinkedList *users = ((ChannelUsers*)channelUsers)->users;
    Node *node = find_node(users, user);

    if (node != NULL) {
        set_data(node, user);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }
}

void register_user(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    add_user_to_hash_table(session, user);

    UserChannels *userChannels = create_user_channels(user);
    add_user_channels(session, userChannels);
}

void register_new_channel_join(Session *session, Channel *channel, User *user) {

    if (session == NULL || channel == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    add_channel_to_hash_table(session, channel);

    ChannelUsers *channelUsers = create_channel_users(channel);
    add_channel_users(session, channelUsers);
    add_user_to_channel_users(channelUsers, user);

    UserChannels *userChannels = find_user_channels(session, user);
    add_channel_to_user_channels(userChannels, channel);
}

void register_existing_channel_join(Session *session, Channel *channel, User *user) {

    if (session == NULL || channel == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    UserChannels *userChannels = find_user_channels(session, user);
    add_channel_to_user_channels(userChannels, channel);

    ChannelUsers *channelUsers = find_channel_users(session, channel);
    add_user_to_channel_users(channelUsers, user);
}

void register_channel_leave(Session *session, Channel *channel, User *user) {

    if (session == NULL || channel == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    UserChannels *userChannels = find_user_channels(session, user);
    remove_channel_in_user_channels(userChannels, channel);

    ChannelUsers *channelUsers = find_channel_users(session, channel);
    remove_user_in_channel_users(channelUsers, user);
}

void find_removable_channels(void *channel, void *arg) {

    if (channel == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        Session *session;
        Channel *channels[MAX_CHANNELS_PER_USER];
        int count;
    } *data = arg;

    ChannelUsers *channelUsers = find_channel_users(data->session, channel);

    if (channelUsers != NULL && channelUsers->count == 1) {

        data->channels[data->count++] = channel;
    }
}

int is_channel_full(ChannelUsers *channelUsers) {

    if (channelUsers == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }
    int count = channelUsers->count; 
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }
    return count == channelUsers->capacity;
}


int are_user_channels_equal(void *userChannels1, void *userChannels2) {

    int equal = 0;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&userChannelsLock);
    }

    if (userChannels1 != NULL && userChannels2 != NULL) {

        equal = are_users_equal(((UserChannels*)userChannels1)->user, ((UserChannels*)userChannels2)->user);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

    return equal;
}

ReadyList * get_ready_list(Session *session) {

    if (session == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&readyListLock);
    }

    ReadyList *readyList = session->readyList;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&readyListLock);
    }

    return readyList;
}

LinkedList * get_ready_users(ReadyList *readyList) {

    if (readyList == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&readyListLock);
    }

    LinkedList *readyUsers = readyList->readyUsers;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&readyListLock);
    }

    return readyUsers;
}

LinkedList * get_ready_channels(ReadyList *readyList) {

    if (readyList == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&readyListLock);
    }

    LinkedList *readyChannels = readyList->readyChannels;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&readyListLock);
    }

    return readyChannels;
}

int are_channel_users_equal(void *channelUsers1, void *channelUsers2) {

    int equal = 0;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }

    if (channelUsers1 != NULL && channelUsers2 != NULL) {

        equal = are_users_equal(((ChannelUsers*)channelUsers1)->channel, ((ChannelUsers*)channelUsers2)->channel);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return equal;
}

LinkedList * get_channels_from_user_channels(UserChannels *userChannels) {

    if (userChannels == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&userChannelsLock);
    }

    LinkedList *channels = userChannels->channels;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&userChannelsLock);
    }

    return channels;
}

LinkedList * get_users_from_channel_users(ChannelUsers *channelUsers) {

    if (channelUsers == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }

    LinkedList *users = channelUsers->users;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return users;
}

LinkedList * get_channel_users_ll(Session *session) {

    if (session == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }

    LinkedList *channelUsersLL = session->channelUsersLL;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }

    return channelUsersLL;
}

int get_channel_users_count(ChannelUsers *channelUsers) {
    
    if (channelUsers == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&channelUsersLock);
    }

    int count = channelUsers->count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&channelUsersLock);
    }
    return count;
}
