#ifndef USER_H
#define USER_H

#define MAX_CHANNELS_PER_USER 5

typedef struct User User;

User * create_user(const char *nickname, const char *username, const char *hostname, const char *realname, int fd);
void delete_user(void *user);

void add_message_to_user_queue(User *user, void *message);

void set_user_data(User *user, const char *username, const char *hostname, const char *realname);
int are_users_equal(void *user1, void *user2);

const char * get_user_nickname(User *user);
const char * get_username(User *user);
const char * get_hostname(User *user);
const char * get_realname(User *user);

#endif
