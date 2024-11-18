#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_user.h"
#else
#include "user.h"
#endif

#include "main.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

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
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char username[MAX_USERNAME_LEN + 1];
    char hostname[MAX_HOSTNAME_LEN + 1];
    char realname[MAX_REALNAME_LEN + 1];
    Queue *outQueue;
    pthread_rwlock_t userLock;
    User *next;
};

#endif

User * create_user(const char *nickname, const char *username, const char *hostname, const char *realname, int fd) {

    User *user = (User*) malloc(sizeof(User));
    if (user == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    user->fd = fd;

    user->outQueue = create_queue(MSG_QUEUE_LEN, sizeof(RegMessage));

    if (is_valid_name(nickname, 0)) {

        safe_copy(user->nickname, sizeof(user->nickname), nickname);
    }
    safe_copy(user->username, sizeof(user->username), username);
    safe_copy(user->hostname, sizeof(user->hostname), hostname);
    safe_copy(user->realname, sizeof(user->realname), realname);
    pthread_rwlock_init(&user->userLock, NULL);

    user->next = NULL; 

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

    User *userCopy = create_user(user->nickname, user->username, user->hostname, user->realname, user->fd);

    return userCopy;
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

void set_user_data(User *user, const char *username, const char *hostname, const char *realname) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    safe_copy(user->username, sizeof(user->username), username);
    safe_copy(user->hostname, sizeof(user->hostname), hostname);
    safe_copy(user->realname, sizeof(user->realname), realname);
}

int are_users_equal(void *user1, void *user2) {

    int equal = 0;

    if (user1 != NULL && user2 != NULL) {
        equal = strcmp(((User*)user1)->nickname, ((User*)user2)->nickname) == 0;
    }
    return equal;
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

void create_user_info(char *buffer, int size, void *user) {

    if (buffer == NULL || user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *nickname = ((User*)user)->nickname;
    const char *username = ((User*)user)->username;
    const char *hostname = ((User*)user)->hostname;

    const char *tokens[] = {nickname, "!", username, "@", hostname};  
    int userInfoLength = 0;

    iterate_string_list(tokens, ARR_SIZE(tokens), add_string_length, &userInfoLength);

    if (userInfoLength <= size) {
        concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), "");
    }
    else {
        LOG(ERROR, "Max message length exceeded");
    }
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

    if (is_valid_name(nickname, 0)) {

        safe_copy(user->nickname, sizeof(user->nickname), nickname);
    }
}

const char * get_username(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->username;
}

const char * get_hostname(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->hostname;
}

const char * get_realname(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->realname;
}

int get_user_fd(User *user) {

    if (user == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return user->fd;  
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