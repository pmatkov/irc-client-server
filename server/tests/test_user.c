#include "../src/test_user.h"

#include <check.h>

START_TEST(test_create_users_table) {

    UsersTable *usersTable = create_users_table(5);

    ck_assert_ptr_ne(usersTable, NULL);
    ck_assert_int_eq(usersTable->allocatedSize, 5);
    ck_assert_int_eq(usersTable->usedSize, 0);
    ck_assert_int_eq(usersTable->addedLinks, 0);

    delete_users_table(usersTable);
}
END_TEST

START_TEST(test_user_get_nickname) {

    UsersTable *usersTable = create_users_table(5);

    User *user = create_user(usersTable, -1, "john");

    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user_get_nickname(user), "john");

    delete_users_table(usersTable);
}
END_TEST

START_TEST(test_user_set_nickname) {

    UsersTable *usersTable = create_users_table(5);

    User *user = create_user(usersTable, -1, "john");

    user_set_nickname(usersTable, user, "mark");

    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user_get_nickname(user), "mark");

    delete_users_table(usersTable);
}
END_TEST

START_TEST(test_create_user) {

    UsersTable *usersTable = create_users_table(5);
    
    ck_assert_ptr_ne(usersTable, NULL);

    User *user = create_user(usersTable, -1, NULL);

    ck_assert_ptr_ne(user, NULL);
    ck_assert_ptr_ne(user->messageQueue, NULL);
    ck_assert_int_eq(user->socketFd, -1);
    ck_assert_str_eq(user->nickname, "user_1");

    user = create_user(usersTable, 2, "john");

    ck_assert_ptr_ne(user, NULL);
    ck_assert_ptr_ne(user->messageQueue, NULL);
    ck_assert_int_eq(user->socketFd, 2);
    ck_assert_str_eq(user->nickname, "john");

    delete_users_table(usersTable);
    delete_user(user);
}
END_TEST

START_TEST(test_remove_user) {

    UsersTable *usersTable = create_users_table(5);

    ck_assert_ptr_ne(usersTable, NULL);

    insert_user(usersTable, 2, "john1");
    insert_user(usersTable, 3, NULL);

    ck_assert_int_eq(usersTable->usedSize, 2);

    int removed = remove_user(usersTable, "john3");
    ck_assert_int_eq(removed, 0);

    removed = remove_user(usersTable, "john1");
    ck_assert_int_eq(removed, 1);

    removed = remove_user(usersTable, "user_2");
    ck_assert_int_eq(removed, 1);

    ck_assert_int_eq(usersTable->usedSize, 0);

    delete_users_table(usersTable);
}
END_TEST

START_TEST(test_insert_user) {

    UsersTable *usersTable = create_users_table(5);

    ck_assert_ptr_ne(usersTable, NULL);

    int inserted = insert_user(usersTable, 2, "john1");

    ck_assert_int_eq(inserted, 1);
    ck_assert_int_eq(usersTable->usedSize, 1);

    User *user = lookup_user(usersTable, "john1");
    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user->nickname, "john1");

    delete_users_table(usersTable);
}
END_TEST

START_TEST(test_lookup_user) {

    UsersTable *usersTable = create_users_table(5);

    User *user = lookup_user(usersTable, "john");
    ck_assert_ptr_eq(user, NULL);

    int inserted = insert_user(usersTable, 2, "john");

    ck_assert_int_eq(inserted, 1);
    ck_assert_int_eq(usersTable->usedSize, 1);

    user = lookup_user(usersTable, "john");
    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user->nickname, "john");

    delete_users_table(usersTable);
}
END_TEST


START_TEST(test_calculate_hash) {

    unsigned hash1 = calculate_hash("user1");
    unsigned hash2 = calculate_hash("user1");
    unsigned hash3 = calculate_hash("user2");

    ck_assert_int_eq(hash1, hash2);
    ck_assert_int_ne(hash1, hash3);
}
END_TEST

Suite* user_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("User");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_users_table);
    tcase_add_test(tc_core, test_user_get_nickname);
    tcase_add_test(tc_core, test_user_set_nickname);
    tcase_add_test(tc_core, test_create_user);
    tcase_add_test(tc_core, test_remove_user);
    tcase_add_test(tc_core, test_insert_user);
    tcase_add_test(tc_core, test_lookup_user);
    tcase_add_test(tc_core, test_calculate_hash);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = user_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif