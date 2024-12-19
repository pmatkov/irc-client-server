#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_user.h"
#else
#include "user.h"
#include "../../libs/src/common.h"
#endif

#include "config.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/common.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>

#define MSG_QUEUE_LEN 20
#define MAX_CHANNELS 20

#ifndef TEST

struct User {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char username[MAX_CHARS + 1];
    char hostname[MAX_CHARS + 1];
    char realname[MAX_CHARS + 1];
    Queue *outQueue;
    pthread_rwlock_t userLock;
};

#endif

User * create_user(int fd, const char *nickname, const char *username, const char *hostname, const char *realname) {

    User *user = (User*) malloc(sizeof(User));
    if (user == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    user->fd = fd;

    safe_copy(user->nickname, ARRAY_SIZE(user->nickname), nickname);
    safe_copy(user->username, ARRAY_SIZE(user->username), username);
    safe_copy(user->hostname, ARRAY_SIZE(user->hostname), hostname);
    safe_copy(user->realname, ARRAY_SIZE(user->realname), realname);

    user->outQueue = create_queue(MSG_QUEUE_LEN, MAX_CHARS + 1);
    pthread_rwlock_init(&user->userLock, NULL);

    return user;
}

void delete_user(void *user) {

    if (user != NULL) {
        delete_queue(((User*)user)->outQueue);
        pthread_rwlock_destroy(&((User*)user)->userLock);
    }

    free(user);
}

User * copy_user(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return create_user(user->fd, user->nickname, user->username, user->hostname, user->realname);
}

void enqueue_to_user_queue(User *user, void *message) {

    if (user == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&user->userLock);
    }
    enqueue(user->outQueue, message);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&user->userLock);
    }
}

void * dequeue_from_user_queue(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&user->userLock);
    }

    void *message = dequeue(((User *)user)->outQueue);
    
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&user->userLock);
    }

    return message; 
}

bool are_users_equal(void *user1, void *user2) {

    bool equal = 0;

    User *u1 = (User*)user1;
    User *u2 = (User*)user2;

    if (u1 != NULL && u1->nickname != NULL && \
        u2 != NULL && u2->nickname != NULL) {

        equal = strcmp(u1->nickname, u2->nickname) == 0;
    }
    return equal;
}

bool is_valid_user_name(const char *string) {

    bool valid = 0;

    if (is_valid_name(string, "-_\\[]{}|^~")) {
        valid = 1;
    };

    return valid;
}

void add_nickname_to_list(void *user, void *arg) {

    if (user == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        char buffer[MAX_CHARS + 1];
    } *data = arg;

    if (strlen(data->buffer) + strlen(((User*)user)->nickname) + 1 < sizeof(data->buffer)) {

        if (strlen(data->buffer)) {
            strcat(data->buffer, " ");
        }
        strcat(data->buffer, ((User*)user)->nickname);
    }
}

void create_user_info(char *buffer, int size, void *arg) {

    if (buffer == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    User *user = arg;

    const char *tokens[] = {user->nickname, "!", user->username, "@", user->hostname};  
    int userInfoLength = 0;

    iterate_string_list(tokens, ARRAY_SIZE(tokens), add_string_length, &userInfoLength);

    if (userInfoLength < size) {
        concat_tokens(buffer, size, tokens, ARRAY_SIZE(tokens), "");
    }

}

int get_user_fd(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->fd;  
}


const char * get_user_nickname(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->nickname;
}

void set_user_nickname(User *user, const char *nickname) {

    if (user == NULL || nickname == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (is_valid_user_name(nickname)) {

        safe_copy(user->nickname, ARRAY_SIZE(user->nickname),nickname);
    }
}

const char * get_user_username(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->username;
}

const char * get_user_hostname(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->hostname;
}

const char * get_user_realname(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->realname;
}

Queue * get_user_queue(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&user->userLock);
    }
    Queue *queue = user->outQueue;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&user->userLock);
    }

    return queue;
}