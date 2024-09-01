 #ifndef USER_H
#define USER_H

typedef struct User User;
typedef struct UsersTable UsersTable;

UsersTable * create_users_table(int size);
void delete_users_table(UsersTable *usersTable);

char * user_get_nickname(User *user);
int user_set_nickname(UsersTable *usersTable, User *user, char *nickname);

int insert_user(UsersTable *usersTable, int socketFd, char *nickname);
int remove_user(UsersTable *usersTable, char *nickname);
User * lookup_user(UsersTable *usersTable, char *nickname);

#endif