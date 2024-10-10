#ifdef TEST
#include "priv_session.h"
#else
#include "session.h"
#include "../../libs/src/hash_table.h"
#endif

#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define START_USERS 500
#define START_CHANNELS 50

#define MAX_USERS START_USERS * LOAD_FACTOR
#define MAX_CHANNELS START_CHANNELS * LOAD_FACTOR

#ifndef TEST

#define LOAD_FACTOR 2.0

/* keeps track of all channels
 joined by a user*/
struct UserChannels {
    User *user;
    LinkedList *channels;
    int capacity;
    int count;
};

// keeps track of all users in a channel
struct ChannelUsers {
    Channel *channel;
    LinkedList *users;
    int capacity;
    int count;
};

/* keeps track of users and channnels
  with messages to send */ 

struct ReadyList {
    LinkedList *readyUsers;
    LinkedList *readyChannels;
};
 
/* keeps track of all users on the server, 
all channels on the server, all channels
 a user has joined and all users in
 a channel */

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
};

#endif

STATIC void delete_user_channels(void *userChannels);
STATIC void delete_channel_users(void *channelUsers);

Session * create_session(void) {

    Session *session = (Session *) malloc(sizeof(Session));
    if (session == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
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

    return session; 
}

void delete_session(Session *session) {

    if (session != NULL) {

        delete_ready_list(session->readyList);
        delete_hash_table(session->users);
        delete_hash_table(session->channels);
        delete_linked_list(session->userChannelsLL);
        delete_linked_list(session->channelUsersLL);
    }

    free(session);    
}


/* don't manually free user or channel after 
adding them to <hash table>, they will be
 freed automatically when hash table is 
 deleted in delete_session() if DeleteValueFunc 
 is passed to create_hash_table() */

void add_user_to_hash_table(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (session->usersCount < session->usersCapacity && find_item_in_hash_table(session->users, (char*)get_user_nickname(user)) == NULL) {

        HashItem *item = create_hash_item((char*)get_user_nickname(user), user); 

        int inserted = insert_item_to_hash_table(session->users, item);

        if (inserted) {
            session->usersCount++;
        }
    }
}

void add_channel_to_hash_table(Session *session, Channel *channel) {

    if (session == NULL || channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (session->channelsCount < session->channelsCapacity && find_item_in_hash_table(session->channels, (char*) get_channel_name(channel)) == NULL) {

        HashItem *item = create_hash_item((char*)get_channel_name(channel), channel); 

        int inserted = insert_item_to_hash_table(session->channels, item);

        if (inserted) {
            session->channelsCount++;
        }

    }
} 

void remove_user_from_hash_table(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (session->usersCount) {

        int removed = remove_item_from_hash_table(session->users, (char*)get_user_nickname(user));

        if (removed) {
            session->usersCount--;
        }
    }
}

void remove_channel_from_hash_table(Session *session, Channel *channel) {

    if (session == NULL || channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (session->channelsCount) {

        int removed = remove_item_from_hash_table(session->channels, (char*)get_channel_name(channel));

        if (removed) {
            session->channelsCount--;
        }
    }
}

User * find_user_in_hash_table(Session *session, const char *nickname) {

    if (session == NULL || nickname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    HashItem *item = find_item_in_hash_table(session->users, (char*)nickname);
    User *user = get_value(item);

    return user;
}

Channel * find_channel_in_hash_table(Session *session, const char *name) {

    if (session == NULL || name == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    HashItem *item = find_item_in_hash_table(session->channels, (char*)name);
    Channel *channel = get_value(item);

    return channel;
}

void change_user_in_hash_table(Session *session, User *oldUser, User *newUser) {

    if (session == NULL || oldUser == NULL || newUser == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    remove_user_from_hash_table(session, oldUser);
    add_user_to_hash_table(session, newUser);

}

ReadyList * create_ready_list(void) {
    
    ReadyList *readyList = (ReadyList *) malloc(sizeof(ReadyList));
    if (readyList == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
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

void add_user_to_ready_users(void *user, void *readyList) {

    if (user == NULL || readyList == NULL ) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!find_node(((ReadyList*)readyList)->readyUsers, user)) {

        Node *node = create_node(user);
        append_node(((ReadyList*)readyList)->readyUsers, node);
    }
}

void add_channel_to_ready_channels(void *channel, void *readyList) {

    if (channel == NULL || readyList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!find_node(((ReadyList*)readyList)->readyChannels, channel)) {

        Node *node = create_node(channel);
        append_node(((ReadyList*)readyList)->readyChannels, node);
    }
}

UserChannels * create_user_channels(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    UserChannels *userChannels = (UserChannels*) malloc(sizeof(UserChannels));
    if (userChannels == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    userChannels->user = user;
    userChannels->channels = create_linked_list(are_channels_equal, NULL);
    userChannels->capacity = MAX_CHANNELS_PER_USER;
    userChannels->count = 0;

    return userChannels;
}

ChannelUsers * create_channel_users(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    ChannelUsers *channelUsers = (ChannelUsers*) malloc(sizeof(ChannelUsers));
    if (channelUsers == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
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
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = create_node(userChannels);
    append_node(session->userChannelsLL, node);

}

void add_channel_users(Session *session, ChannelUsers *channelUsers) {

    if (session == NULL || channelUsers == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = create_node(channelUsers);
    append_node(session->channelUsersLL, node);

}

int remove_user_channels(Session *session, UserChannels *userChannels) {

    if (session == NULL || userChannels == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return remove_node(session->userChannelsLL, userChannels);
}

int remove_channel_users(Session *session, ChannelUsers *channelUsers) {

    if (session == NULL || channelUsers == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return remove_node(session->channelUsersLL, channelUsers);
}

UserChannels * find_user_channels(Session *session, User *user) {

    if (session == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = find_node(session->userChannelsLL, &(UserChannels){user, NULL, 0, 0});

    UserChannels *userChannels = get_data(node);

    return userChannels;
}

ChannelUsers * find_channel_users(Session *session, Channel *channel) {

    if (session == NULL || channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = find_node(session->channelUsersLL, &(ChannelUsers){channel, NULL, 0, 0});

    ChannelUsers *channelUsers = get_data(node);

    return channelUsers;
}

int add_channel_to_user_channels(UserChannels *userChannels, Channel *channel) {

    if (userChannels == NULL || channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int added = 0;

    if (userChannels->count < userChannels->capacity) {

        Node *node = create_node(channel);
        append_node(userChannels->channels, node);
        userChannels->count++;
        added = 1;
    }

    return added;
}

int add_user_to_channel_users(ChannelUsers *channelUsers, User *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int added = 0;

    if (channelUsers->count < channelUsers->capacity) {

        Node *node = create_node(user);
        append_node(channelUsers->users, node);
        channelUsers->count++;
        added = 1;
    }

    return added;
}

Channel * find_channel_in_user_channels(UserChannels *userChannels, Channel *channel) {

    if (userChannels == NULL || channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = find_node(userChannels->channels, channel);

    Channel *foundChannel = get_data(node);

    return foundChannel;
}

User * find_user_in_channel_users(ChannelUsers *channelUsers, User *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = find_node(channelUsers->users, user);

    User *foundUser = get_data(node);

    return foundUser;
}

void remove_channel_in_user_channels(UserChannels *userChannels, Channel *channel) {

    if (userChannels == NULL || channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    remove_node(userChannels->channels, channel);
    userChannels->count--;
}

void remove_user_in_channel_users(ChannelUsers *channelUsers, User *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    remove_node(channelUsers->users, user);
    channelUsers->count--;
}

void change_user_in_user_channels(UserChannels *userChannels, User *user) {

    if (userChannels == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    userChannels->user = user;
}

void change_user_in_channel_users(void *channelUsers, void *user) {

    if (channelUsers == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    LinkedList *users = ((ChannelUsers*)channelUsers)->users;

    Node *node = find_node(users, user);

    if (node != NULL) {

        set_data(node, user);
    }
}

void find_removable_channels(void *channel, void *arg) {

    if (channel == NULL || arg == NULL) {
        FAILED(NULL, ARG_ERROR);
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
        FAILED(NULL, ARG_ERROR);
    }

    return channelUsers->count == channelUsers->capacity;
}


int are_user_channels_equal(void *userChannels1, void *userChannels2) {

    int equal = 0;

    if (userChannels1 != NULL && userChannels2 != NULL) {

        equal = are_users_equal(((UserChannels*)userChannels1)->user, ((UserChannels*)userChannels2)->user);
    }

    return equal;
}

ReadyList * get_ready_list(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return session->readyList;
}

LinkedList * get_ready_users(ReadyList *readyList) {

    if (readyList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return readyList->readyUsers;
}

LinkedList * get_ready_channels(ReadyList *readyList) {

    if (readyList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return readyList->readyChannels;
}

int are_channel_users_equal(void *channelUsers1, void *channelUsers2) {

    int equal = 0;

    if (channelUsers1 != NULL && channelUsers2 != NULL) {

        equal = are_users_equal(((ChannelUsers*)channelUsers1)->channel, ((ChannelUsers*)channelUsers2)->channel);
    }

    return equal;
}

LinkedList * get_channels_from_user_channels(UserChannels *userChannels) {

    if (userChannels == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return userChannels->channels;
}

LinkedList * get_users_from_channel_users(ChannelUsers *channelUsers) {

    if (channelUsers == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channelUsers->users;
}

LinkedList * get_channel_users_ll(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return session->channelUsersLL;
}

int get_users_count_from_channel_users(ChannelUsers *channelUsers) {
    
    if (channelUsers == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return channelUsers->count;
}
