#ifndef USER_H
#define USER_H

#include "../../shared/src/queue.h"

#define MAX_NICKNAME_LEN 9

typedef struct User {
    MessageQueue *messageQueue;
    int socketFd;
    char nickname[MAX_NICKNAME_LEN + 1];
    struct User *next;
} User;

typedef struct {
    User **users;
    int allocatedSize;
    int usedSize;
    int addedLinks;
} UsersTable;

UsersTable * create_users_table(int size);
void delete_users_table(UsersTable *usersTable);

char * user_get_nickname(User *user);
int user_set_nickname(UsersTable *usersTable, User *user, char *nickname);

int insert_user(UsersTable *usersTable, int socketFd, char *nickname);
int remove_user(UsersTable *usersTable, char *nickname);
User * lookup_user(UsersTable *usersTable, char *nickname);

#ifdef TEST

User * create_user(UsersTable *usersTable, int socketFd, char *nickname);
void delete_user(User *user);
unsigned calculate_hash(const char *string);

#endif

#endif