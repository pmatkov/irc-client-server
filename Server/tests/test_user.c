#include "../src/user.h"

#include <check.h>

START_TEST(test_calculate_hash) {

    unsigned hash1 = calculate_hash("user1");
    unsigned hash2 = calculate_hash("user1");
    unsigned hash3 = calculate_hash("user2");

    ck_assert_int_eq(hash1, hash2);
    ck_assert_int_ne(hash1, hash3);
}
END_TEST

START_TEST(test_create_hash_table) {

    UsersHashTable *ht = create_hash_table(5);

    ck_assert_ptr_ne(ht, NULL);
    ck_assert_int_eq(ht->allocatedSize, 5);
    ck_assert_int_eq(ht->usedSize, 0);
    ck_assert_int_eq(ht->addedLinks, 0);

    delete_hash_table(ht);
}
END_TEST

START_TEST(test_uint_to_str) {

    char buffer[5] = {'\0'};

    int status = uint_to_str(buffer, 3, 10);

    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "10");

    status = uint_to_str(buffer, 5, 100);
    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "100");
    
    status = uint_to_str(buffer, 2, 10);

    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(buffer, "100");

    status = uint_to_str(buffer, 0, 100);
    ck_assert_int_eq(status, 0);

    char *nullPtr = NULL;

    status = uint_to_str(nullPtr, 0, 100);
    ck_assert_int_eq(status, 0);

}
END_TEST

START_TEST(test_lookup_user) {

    UsersHashTable *ht = create_hash_table(5);

    User *user = lookup_user(ht, NULL);
    ck_assert_ptr_eq(user, NULL);

    user = lookup_user(ht, "john");
    ck_assert_ptr_eq(user, NULL);

    delete_hash_table(ht);
}
END_TEST

START_TEST(test_create_user) {

    UsersHashTable *ht = create_hash_table(5);

    User *user = create_user(ht, -1, NULL);

    ck_assert_ptr_ne(user, NULL);
    ck_assert_ptr_ne(user->messageQueue, NULL);
    ck_assert_int_eq(user->socketFd, -1);
    ck_assert_str_eq(user->nickname, "user_1");

    user = create_user(ht, 2, "john");

    ck_assert_ptr_ne(user, NULL);
    ck_assert_ptr_ne(user->messageQueue, NULL);
    ck_assert_int_eq(user->socketFd, 2);
    ck_assert_str_eq(user->nickname, "john");

    delete_hash_table(ht);
    delete_user(user);
}
END_TEST

START_TEST(test_insert_user) {

    UsersHashTable *ht = create_hash_table(5);

    int inserted1 = insert_user(ht, 2, "john1");
    int inserted2 = insert_user(ht, 3, "john2");

    ck_assert_ptr_ne(ht, NULL);
    ck_assert_int_eq(ht->usedSize, 2);
    ck_assert_int_eq(ht->allocatedSize, 5);
    ck_assert_int_eq(inserted1, 1);
    ck_assert_int_eq(inserted2, 1);

    User *user = lookup_user(ht, NULL);
    ck_assert_ptr_eq(user, NULL);

    user = lookup_user(ht, "john1");
    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user->nickname, "john1");

    delete_hash_table(ht);
}
END_TEST


START_TEST(test_remove_user) {

    UsersHashTable *ht = create_hash_table(5);

    int inserted1 = insert_user(ht, 2, "john1");
    int inserted2 = insert_user(ht, 3, NULL);

    ck_assert_ptr_ne(ht, NULL);
    ck_assert_int_eq(ht->usedSize, 2);
    ck_assert_int_eq(inserted1, 1);
    ck_assert_int_eq(inserted2, 1);

    int status = remove_user(ht, NULL);
    ck_assert_int_eq(status, 0);

    status = remove_user(ht, "john3");
    ck_assert_int_eq(status, 0);

    status = remove_user(ht, "john1");
    ck_assert_int_eq(status, 1);

    status = remove_user(ht, "user_2");
    ck_assert_int_eq(status, 1);

    ck_assert_int_eq(ht->usedSize, 0);

    delete_hash_table(ht);
}
END_TEST

Suite* user_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("User");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_calculate_hash);
    tcase_add_test(tc_core, test_create_hash_table);
    tcase_add_test(tc_core, test_uint_to_str);
    tcase_add_test(tc_core, test_lookup_user);
    tcase_add_test(tc_core, test_create_user);
    tcase_add_test(tc_core, test_insert_user);
    tcase_add_test(tc_core, test_remove_user);

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