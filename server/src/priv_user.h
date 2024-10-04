#ifndef USER_H
#define USER_H

#include "../../shared/src/priv_queue.h"

#define MAX_NICKNAME_LEN 9
#define MAX_USERNAME_LEN 9
#define MAX_HOSTNAME_LEN 63
#define MAX_REALNAME_LEN 63

#define MAX_CHANNELS_PER_USER 5

typedef struct User {
    char nickname[MAX_NICKNAME_LEN + 1];
    char username[MAX_USERNAME_LEN + 1];
    char hostname[MAX_HOSTNAME_LEN + 1];
    char realname[MAX_REALNAME_LEN + 1];
    Queue *outQueue;
    int fd;
    struct User *next;
} User;

User * create_user(const char *nickname, const char *username, const char *hostname, const char *realname, int fd);
void delete_user(void *value);

void add_message_to_user_queue(User *user, void *message);
void * remove_message_from_user_queue(User *user);

void set_user_data(User *user, const char *username, const char *hostname, const char *realname);
int are_users_equal(void *user1, void *user2);

void add_nickname_to_list(void *user, void *namesList);

const char *get_user_nickname(User *user);
const char * get_username(User *user);
const char * get_hostname(User *user);
const char * get_realname(User *user);
Queue * get_user_queue(User *user);
int get_user_fd(User *user);

#endif