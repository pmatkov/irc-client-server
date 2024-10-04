#ifdef TEST
#include "priv_user.h"
#else
#include "user.h"
#endif

#include "../../shared/src/priv_message.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>

#define MAX_NICKNAME_LEN 9
#define MAX_USERNAME_LEN 9
#define MAX_HOSTNAME_LEN 63
#define MAX_REALNAME_LEN 63

#define MSG_QUEUE_LEN 20
#define MAX_CHANNELS 20

#ifndef TEST

struct User {
    char nickname[MAX_NICKNAME_LEN + 1];
    char username[MAX_USERNAME_LEN + 1];
    char hostname[MAX_HOSTNAME_LEN + 1];
    char realname[MAX_REALNAME_LEN + 1];
    Queue *outQueue;
    int fd;
    User *next;
};

#endif

User * create_user(const char *nickname, const char *username, const char *hostname, const char *realname, int fd) {

    User *user = (User*) malloc(sizeof(User));
    if (user == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    user->outQueue = create_queue(MSG_QUEUE_LEN, sizeof(RegMessage));

    if (is_valid_name(nickname, 0)) {

        safe_copy(user->nickname, MAX_NICKNAME_LEN + 1, nickname);
    }
    safe_copy(user->username, MAX_USERNAME_LEN + 1, username);
    safe_copy(user->hostname, MAX_HOSTNAME_LEN + 1, hostname);
    safe_copy(user->realname, MAX_REALNAME_LEN + 1, realname);

    user->fd = fd;
    user->next = NULL; 

    return user;
}

void delete_user(void *user) {

    if (user != NULL) {
        delete_queue(((User*)user)->outQueue);
    }

    free(user);
}

void add_message_to_user_queue(User *user, void *message) {

    if (user == NULL || message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    
    enqueue(user->outQueue, message);
}

void * remove_message_from_user_queue(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage *message = NULL;

    message = dequeue(((User *)user)->outQueue);

    return message; 
}

void set_user_data(User *user, const char *username, const char *hostname, const char *realname) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    safe_copy(user->username, MAX_USERNAME_LEN + 1, username);
    safe_copy(user->hostname, MAX_HOSTNAME_LEN + 1, hostname);
    safe_copy(user->realname, MAX_REALNAME_LEN + 1, realname);
}

int are_users_equal(void *user1, void *user2) {

    int equal = 0;

    if (user1 != NULL && user2 != NULL) {
        equal = strcmp(((User*)user1)->nickname, ((User*)user2)->nickname) == 0;
    }
    return equal;
}


void add_nickname_to_list(void *user, void *namesList) {

    if (user == NULL || namesList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    add_string_to_string_list((StringList*)namesList, ((User*)user)->nickname);

}

const char *get_user_nickname(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return user->nickname;
}

const char * get_username(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return user->username;
}

const char * get_hostname(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return user->hostname;
}

const char * get_realname(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return user->realname;
}

Queue * get_user_queue(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return user->outQueue;
}

int get_user_fd(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return user->fd;  
}