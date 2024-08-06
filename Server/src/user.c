#include "user.h"
#include "dispatcher.h"
#include "main.h"

#include <stdlib.h>
#include <string.h>

STATIC unsigned calculate_hash(const char *string);
STATIC int uint_to_str(char *string, int size, unsigned number);
STATIC User * create_user(UsersHashTable *usersHashTable, int socketFd, char *nickname);
STATIC void delete_user(User *user);

STATIC unsigned calculate_hash(const char *string) {

    unsigned long hash = 5381;
    int c;

    while ((c = *string++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return (unsigned) hash;
}

UsersHashTable * create_hash_table(int size) {

    UsersHashTable *usersHashTable = (UsersHashTable *) malloc(sizeof(UsersHashTable));
    if (usersHashTable == NULL) {
        failed("Error allocating memory.");
    }

    usersHashTable->users = (User **) malloc(size * sizeof(User*));
    if (usersHashTable->users == NULL) {
        failed("Error allocating memory.");
    }

    for (int i = 0; i < size; i++) {
        usersHashTable->users[i] = NULL;

    }
    usersHashTable->allocatedSize = size;
    usersHashTable->usedSize = 0;
    usersHashTable->addedLinks = 0;

    return usersHashTable;
}

void delete_hash_table(UsersHashTable *usersHashTable) {

    if (usersHashTable != NULL) {

        for (int i = 0; i < usersHashTable->allocatedSize; i++) {

            // if (usersHashTable->users[i] != NULL) {
            //     delete_message_queue(usersHashTable->users[i]->messageQueue);
            // }
            // free(usersHashTable->users[i]);

            User *current = usersHashTable->users[i];

            while (current != NULL) {

                User *next = current->next;
                if (current->messageQueue != NULL) {
                    delete_message_queue(current->messageQueue);
                }
                free(current);
                current = next;
            }
        }
        free(usersHashTable->users);
        free(usersHashTable);
    }
}

STATIC int uint_to_str(char *string, int size, unsigned number) {

    if (string == NULL || !size) {
        return 0;
    }

    unsigned numberCp = number;
    int digits = 0;

    do {
        numberCp /= 10;
        digits++;
    } while (numberCp);

    if (size < digits + 1) {
        return 0;
    }

    for (int i = 0; i < digits; i++) {
        string[digits-i-1] = number % 10 + '0';
        number /= 10;
    }

    string[digits] = '\0';

    return 1;
}

STATIC User * create_user(UsersHashTable *usersHashTable, int socketFd, char *nickname) {

    User *user = (User*) malloc(sizeof(User));
    if (user == NULL) {
        failed("Error allocating memory.");
    }

    user->messageQueue = create_message_queue(MSG_QUEUE_LEN);
    user->socketFd = socketFd;
    if (!set_nickname(usersHashTable, user, nickname)) {
        return NULL;
    }
    user->next = NULL; 

    return user;
}

STATIC void delete_user(User *user) {

    free(user);
}


int set_nickname(UsersHashTable *usersHashTable, User *user, char *nickname) {

    if (nickname == NULL || strnlen(nickname, MAX_NICKNAME_LEN + 1) == MAX_NICKNAME_LEN + 1 || \
    lookup_user(usersHashTable, nickname) != NULL) {

        strcpy(user->nickname, "user_");

        const int FIRST_DIG_INDEX = 5;
        const int MAX_DIGITS = 4;

        if (!uint_to_str(&user->nickname[FIRST_DIG_INDEX], MAX_DIGITS + 1, usersHashTable->usedSize + usersHashTable->addedLinks + 1)) {
            return 0;
        }
    }
    else {
        strcpy(user->nickname, nickname);
    }

    return 1;
}

int insert_user(UsersHashTable *usersHashTable, int socketFd, char *nickname) {

    User *user = create_user(usersHashTable, socketFd, nickname);

    if (user == NULL) {
        return 0;
    }

    unsigned index = calculate_hash(user->nickname) % usersHashTable->allocatedSize;

    if (usersHashTable->users[index] == NULL) {

        usersHashTable->users[index] = user;
        usersHashTable->usedSize++;
    }
    else {
        User *temp = usersHashTable->users[index];

        while (temp->next) {
            temp = temp->next;
        }
        
        temp->next = user;
        usersHashTable->addedLinks++;
    }

    return 1;
}

int remove_user(UsersHashTable *usersHashTable, char *nickname) {

    if (nickname == NULL || strnlen(nickname, MAX_NICKNAME_LEN + 1) == MAX_NICKNAME_LEN + 1) {
        return 0;
    }

    unsigned index = calculate_hash(nickname) % usersHashTable->allocatedSize;

    User *temp = usersHashTable->users[index];
    User *previousUser = NULL;

    while (temp && strcmp(temp->nickname, nickname) != 0) {

        previousUser = temp;
        temp = temp->next;
    }

    if (!temp) {
        return 0;
    }
    else if (!previousUser) {
        
        usersHashTable->users[index] = temp->next;

        if (!usersHashTable->users[index]) {
            usersHashTable->usedSize--;
        }
        else {
            usersHashTable->addedLinks--;
        }
    }
    else {
        previousUser->next = temp->next;
        usersHashTable->addedLinks--;
    }
    delete_user(temp);
   
    return 1;
}


User * lookup_user(UsersHashTable *usersHashTable, char *nickname) {
    
    if (nickname == NULL || strnlen(nickname, MAX_NICKNAME_LEN + 1) == MAX_NICKNAME_LEN + 1) {
        return NULL;
    }

    unsigned index = calculate_hash(nickname) % usersHashTable->allocatedSize;
    User *temp = usersHashTable->users[index];

    while (temp && strcmp(temp->nickname, nickname) != 0) {

        temp = temp->next;
    }

    return temp;
}
