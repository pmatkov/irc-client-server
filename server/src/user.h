#ifndef USER_H
#define USER_H

#include "../../libs/src/queue.h"

#include <stdbool.h>
#include <pthread.h>

#define MAX_CHANNELS_PER_USER 5

typedef struct User User;

User * create_user(int fd, const char *nickname, const char *username, const char *hostname, const char *realname);
void delete_user(void *user);

User * copy_user(User *user);

void enqueue_to_user_queue(User *user, void *message);
void * dequeue_from_user_queue(User *user);

bool are_users_equal(void *user1, void *user2);
bool is_valid_user_name(const char *string);

void add_nickname_to_list(void *user, void *namesList);
void create_user_info(char *buffer, int size, void *arg);

int get_user_fd(User *user);
const char * get_user_nickname(User *user);
void set_user_nickname(User *user, const char *nickname);
const char * get_user_username(User *user);
const char * get_user_hostname(User *user);
const char * get_user_realname(User *user);
Queue * get_user_queue(User *user);

#endif
