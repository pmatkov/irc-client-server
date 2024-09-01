#ifdef TEST
#include "test_user.h"
#else
#include "user.h"
#endif

#include "../../shared/src/queue.h"
#include "../../shared/src/parser.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MSG_QUEUE_LEN 20
#define MAX_NICKNAME_LEN 9
#define DEFAULT_USERS 1020

#ifndef TEST

struct User {
    MessageQueue *messageQueue;
    int socketFd;
    char nickname[MAX_NICKNAME_LEN + 1];
    User *next;
};

struct UsersTable {
    User **users;
    int allocatedSize;
    int usedSize;
    int addedLinks;
};

#endif

STATIC User * create_user(UsersTable *usersTable, int socketFd, char *nickname);
STATIC void delete_user(User *user);
STATIC unsigned calculate_hash(const char *string);


UsersTable * create_users_table(int size) {

    if (size <= 0) {
        size = DEFAULT_USERS;
    }

    UsersTable *usersTable = (UsersTable *) malloc(sizeof(UsersTable));
    if (usersTable == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    usersTable->users = (User **) malloc(size * sizeof(User*));
    if (usersTable->users == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    for (int i = 0; i < size; i++) {
        usersTable->users[i] = NULL;

    }
    usersTable->allocatedSize = size;
    usersTable->usedSize = 0;
    usersTable->addedLinks = 0;

    return usersTable;
}

void delete_users_table(UsersTable *usersTable) {

    if (usersTable != NULL) {

        for (int i = 0; i < usersTable->allocatedSize; i++) {

            User *current = usersTable->users[i];

            while (current != NULL) {

                User *next = current->next;
                if (current->messageQueue != NULL) {
                    delete_message_queue(current->messageQueue);
                }
                free(current);
                current = next;
            }
        }
        free(usersTable->users);
    }
    free(usersTable);
}

int user_set_nickname(UsersTable *usersTable, User *user, char *nickname) {

    if (usersTable == NULL || user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (nickname == NULL || strnlen(nickname, MAX_NICKNAME_LEN + 1) == MAX_NICKNAME_LEN + 1 || \
    lookup_user(usersTable, nickname) != NULL) {

        strcpy(user->nickname, "user_");

        const int FIRST_DIG_INDEX = 5;
        const int MAX_DIGITS = 4;

        if (!uint_to_str(&user->nickname[FIRST_DIG_INDEX], MAX_DIGITS + 1, usersTable->usedSize + usersTable->addedLinks + 1)) {
            return 0;
        }
    }
    else {
        strcpy(user->nickname, nickname);
    }

    return 1;
}

char * user_get_nickname(User *user) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    } 

    return user->nickname;
}

STATIC User * create_user(UsersTable *usersTable, int socketFd, char *nickname) {

    if (usersTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    User *user = (User*) malloc(sizeof(User));
    if (user == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    user->messageQueue = create_message_queue(EXTENDED_MSG, MSG_QUEUE_LEN);
    user->socketFd = socketFd;
    if (!user_set_nickname(usersTable, user, nickname)) {
        return NULL;
    }
    user->next = NULL; 

    return user;
}

STATIC void delete_user(User *user) {

    free(user);
}

int insert_user(UsersTable *usersTable, int socketFd, char *nickname) {

    if (usersTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    User *user = create_user(usersTable, socketFd, nickname);

    if (user == NULL) {
        return 0;
    }

    unsigned index = calculate_hash(user->nickname) % usersTable->allocatedSize;

    if (usersTable->users[index] == NULL) {

        usersTable->users[index] = user;
        usersTable->usedSize++;
    }
    else {
        User *current = usersTable->users[index];

        while (current->next != NULL) {
            current = current->next;
        }
        
        current->next = user;
        usersTable->addedLinks++;
    }

    return 1;
}

int remove_user(UsersTable *usersTable, char *nickname) {

    if (usersTable == NULL || nickname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    unsigned index = calculate_hash(nickname) % usersTable->allocatedSize;

    User *current = usersTable->users[index];
    User *previous = NULL;

    while (current != NULL && strcmp(current->nickname, nickname) != 0) {

        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        return 0;
    }
    else if (previous == NULL) {
        
        usersTable->users[index] = current->next;

        if (usersTable->users[index] == NULL) {
            usersTable->usedSize--;
        }
        else {
            usersTable->addedLinks--;
        }
    }
    else {
        previous->next = current->next;
        usersTable->addedLinks--;
    }
    delete_user(current);
   
    return 1;
}


User * lookup_user(UsersTable *usersTable, char *nickname) {
    
    if (usersTable == NULL || nickname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    unsigned index = calculate_hash(nickname) % usersTable->allocatedSize;
    User *current = usersTable->users[index];

    while (current != NULL && strcmp(current->nickname, nickname) != 0) {

        current = current->next;
    }

    return current;
}

STATIC unsigned calculate_hash(const char *string) {

    unsigned long hash = 5381;
    int c;

    while ((c = *string++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return (unsigned) hash;
}
