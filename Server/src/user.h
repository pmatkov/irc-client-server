#ifndef USER_H
#define USER_H

#include "dispatcher.h"

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

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
} UsersHashTable;

UsersHashTable * create_hash_table(int size);
void delete_hash_table(UsersHashTable *usersHashTable);

int set_nickname(UsersHashTable *usersHashTable, User *user, char *nickname);

int insert_user(UsersHashTable *usersHashTable, int socketFd, char *nickname);
int remove_user(UsersHashTable *usersHashTable, char *nickname);
User * lookup_user(UsersHashTable *usersHashTable, char *nickname);

STATIC unsigned calculate_hash(const char *string);
STATIC int uint_to_str(char *string, int size, unsigned number);
STATIC User * create_user(UsersHashTable *usersHashTable, int socketFd, char *nickname);
STATIC void delete_user(User *user);

#endif