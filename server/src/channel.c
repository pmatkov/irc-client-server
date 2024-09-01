#ifdef TEST
#include "test_channel.h"
#else
#include "channel.h"
#endif

#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>

#define MAX_CHANNELS 20
#define MAX_USERS 20

#define MAX_CHANNEL_NAME 50
#define MSG_QUEUE_LEN 20
#define DELETED (User*) (0xFFFFFFFFFFFFFFFFUL)

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct Channel {
    MessageQueue *messageQueue;
    char name[MAX_CHANNEL_NAME + 1];
    User **users;
    ChannelType channelType;
    int usersCount;
    Channel *next;  
};

struct ChannelList {
    Channel *head;
    int channelCount;
};

#endif

STATIC Channel * create_channel(char *name, ChannelType channelType);
STATIC void delete_channel(Channel *channel);

STATIC int is_channel_empty(Channel *channel);
STATIC int is_channel_full(Channel *channel);

STATIC int is_channel_list_empty(ChannelList *channelListl);
STATIC int is_channel_list_full(ChannelList *channelListl);

STATIC int is_valid_channel_type(ChannelType type);

STATIC int is_valid_channel_type(ChannelType type) {

    return type >= 0 && type < CHANNEL_TYPE_COUNT;
}

ChannelList *create_channel_list(void) {

    ChannelList *channelList = (ChannelList *) malloc(sizeof(ChannelList));
    if (channelList == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    channelList->head = NULL;
    channelList->channelCount = 0;

    return channelList;
}

void delete_channel_list(ChannelList *channelList) {

    if (channelList != NULL) {
        Channel *current = channelList->head;
        Channel *previous = NULL;

        while (current != NULL) {
            previous = current;
            current = current->next;
            
            delete_channel(previous);
        }
    }

}

STATIC Channel * create_channel(char *name, ChannelType channelType) {

    if (name == NULL || strnlen(name, MAX_CHANNEL_NAME + 1) == MAX_CHANNEL_NAME + 1 || !is_valid_channel_type(channelType)) {
        FAILED(NULL, ARG_ERROR);
    }

    Channel *channel = (Channel *) malloc(sizeof(Channel));
    if (channel == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    channel->messageQueue = create_message_queue(EXTENDED_MSG, MSG_QUEUE_LEN);
    strcpy(channel->name, name);

    channel->users = (User **) malloc(MAX_USERS * sizeof(User*));
    if (channel->users == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }
    channel->channelType = channelType;
    channel->usersCount = 0;
    channel->next = NULL;

    return channel;
}

STATIC void delete_channel(Channel *channel) {

    if (channel != NULL) {

        delete_message_queue(channel->messageQueue);
        free(channel->users);
    }

    free(channel);
}

STATIC int is_channel_list_empty(ChannelList *channelList) {

    if (channelList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return channelList->channelCount == 0;
}

STATIC int is_channel_list_full(ChannelList *channelList) {

    if (channelList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return channelList->channelCount == MAX_CHANNEL_NAME;
}


Channel * add_channel(ChannelList *channelList, char *name, ChannelType channelType) {

    if (channelList == NULL || name == NULL || !is_valid_channel_type(channelType)) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!is_channel_list_full(channelList) && lookup_channel(channelList, name) == NULL) {

        Channel *channel = create_channel(name, channelType);

        channel->next = channelList->head;
        channelList->head = channel;

        channelList->channelCount++;

        return channel;
    }
    return NULL;
}

int remove_channel(ChannelList *channelList, char *name) {

    if (channelList == NULL || name == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!is_channel_list_empty(channelList)) {

        Channel *current = channelList->head;
        Channel *previous = NULL;

        while (current != NULL && strcmp(current->name, name) != 0) {
            previous = current;
            current = current->next;
        }

        if (current != NULL) {
            
            if (previous == NULL) {
                channelList->head = channelList->head->next;
            }
            else {
                previous->next = current->next;
            }

            delete_channel(current);
            channelList->channelCount--;

            return 1;
        }
    }
    return 0;
}

Channel *lookup_channel(ChannelList *channelList, char *name) {

    if (channelList == NULL || name == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Channel *current = channelList->head;

    while (current && strcmp(current->name, name) != 0) {
        current = current->next;
    }

    return current;
}

int add_user_to_channel(Channel *channel, User *user) {

    if (channel == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!is_channel_full(channel))  {

        int i = 0;

        while (channel->users[i] != NULL && channel->users[i] != DELETED) {
            i++;
        }
            
        channel->users[i] = user;
        channel->usersCount++;
        return 1;
    }
    return 0;
}

int remove_user_from_channel(Channel *channel, char *nickname) {

    if (channel == NULL || nickname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0, removed = 0;

    if (!is_channel_empty(channel))  {

        while (channel->users[i] != NULL) {
        
            if (strcmp(user_get_nickname(channel->users[i]), nickname) == 0) {

                channel->users[i] = DELETED;
                channel->usersCount--;
                removed = 1;
                break;
            }
            i++;
        }
    }
    return removed;
}

User * lookup_user_in_channel(Channel *channel, char *nickname) {

    if (channel == NULL || nickname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0, found = 0; 

    while (channel->users[i] != NULL) {

        if (channel->users[i] == DELETED) {
            i++; 
            continue;
        }

        if (strcmp(user_get_nickname(channel->users[i]), nickname) == 0) {
            found = 1;
            break;
        }
        i++;
    }

    return found ? channel->users[i] : NULL;
}

STATIC int is_channel_empty(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return channel->usersCount == 0;
}
STATIC int is_channel_full(Channel *channel) {

    if (channel == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return channel->usersCount == MAX_USERS;
}
