#include "../src/priv_hash_table.h"
#include "../../server/src/priv_user.h"

#include <check.h>
#include <stdlib.h>

#define TABLE_CAPACITY 5

START_TEST(test_create_hash_table) {

    HashTable *hashTable = create_hash_table(TABLE_CAPACITY,string_hash_function, are_strings_equal, NULL, delete_user);

    ck_assert_ptr_ne(hashTable, NULL);
    ck_assert_ptr_ne(hashTable->items, NULL);
    ck_assert_int_eq(hashTable->capacity, TABLE_CAPACITY);
    ck_assert_int_eq(hashTable->itemsCount, 0);
    ck_assert_int_eq(hashTable->linksCount, 0);
    ck_assert_ptr_eq(hashTable->hashFunc, string_hash_function);
    ck_assert_ptr_eq(hashTable->comparatorFunc, are_strings_equal);

    delete_hash_table(hashTable);
}
END_TEST

START_TEST(test_create_hash_item) {

    User *user = create_user("john", NULL, NULL, NULL, 0);
    HashItem *item = create_hash_item(user->nickname, user);

    ck_assert_ptr_ne(user, NULL);
    ck_assert_ptr_ne(item, NULL);
    ck_assert_str_eq(((char*)item->key), "john");
    ck_assert_str_eq(((User*)item->value)->nickname, "john");

    delete_hash_item(item, NULL, delete_user);
}
END_TEST

START_TEST(test_is_hash_table_empty) {

    HashTable *hashTable = create_hash_table(TABLE_CAPACITY,string_hash_function, are_strings_equal, NULL, delete_user);

    int status = is_hash_table_empty(hashTable);
    ck_assert_int_eq(status, 1);     

    delete_hash_table(hashTable);
}
END_TEST

START_TEST(test_is_hash_table_full) {

    HashTable *hashTable = create_hash_table(TABLE_CAPACITY,string_hash_function, are_strings_equal, NULL, delete_user);

    int status = is_hash_table_full(hashTable);
    ck_assert_int_eq(status, 0);     

    delete_hash_table(hashTable);
}
END_TEST

START_TEST(test_insert_item_to_hash_table) {

    HashTable *hashTable = create_hash_table(TABLE_CAPACITY,string_hash_function, are_strings_equal, NULL, delete_user);

    User *user = create_user("john", NULL, NULL, NULL, 0);
    HashItem *item = create_hash_item(user->nickname, user);

    int status = insert_item_to_hash_table(hashTable, item);
    ck_assert_int_eq(status, 1);
    ck_assert_int_eq(hashTable->itemsCount, 1);     

    delete_hash_table(hashTable);
}
END_TEST

START_TEST(test_remove_item_from_hash_table) {

    HashTable *hashTable = create_hash_table(TABLE_CAPACITY,string_hash_function, are_strings_equal, NULL, delete_user);

    User *user = create_user("john", NULL, NULL, NULL, 0);
    HashItem *item = create_hash_item(user->nickname, user);

    insert_item_to_hash_table(hashTable, item);
    int status = remove_item_from_hash_table(hashTable, "john");
    ck_assert_int_eq(status, 1); 
    ck_assert_int_eq(hashTable->itemsCount, 0);         

    delete_hash_table(hashTable);
}
END_TEST

START_TEST(test_find_item_in_hash_table) {

    HashTable *hashTable = create_hash_table(TABLE_CAPACITY,string_hash_function, are_strings_equal, NULL, delete_user);

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    HashItem *item1 = create_hash_item(user1->nickname, user1);
    User *user2 = create_user("mark", NULL, NULL, NULL, 0);
    HashItem *item2 = create_hash_item(user2->nickname, user2);

    insert_item_to_hash_table(hashTable, item1);
    insert_item_to_hash_table(hashTable, item2);

    HashItem *item = find_item_in_hash_table(hashTable, "mark");

    ck_assert_ptr_ne(item, NULL);
    ck_assert_str_eq(((char*)item->key), "mark");
    ck_assert_str_eq(((User*)item->value)->nickname, "mark");      

    delete_hash_table(hashTable);
}
END_TEST

START_TEST(test_string_hash_function) {

    unsigned long hash1 = string_hash_function("user1");
    unsigned long hash2 = string_hash_function("user1");
    unsigned long hash3 = string_hash_function("user2");

    ck_assert_int_eq(hash1, hash2);
    ck_assert_int_ne(hash1, hash3);
}
END_TEST

START_TEST(test_calculate_load_factor) {

    HashTable *hashTable = create_hash_table(2,string_hash_function, are_strings_equal, NULL, delete_user);

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    HashItem *item1 = create_hash_item(user1->nickname, user1);
    User *user2 = create_user("mark", NULL, NULL, NULL, 0);
    HashItem *item2 = create_hash_item(user2->nickname, user2);

    insert_item_to_hash_table(hashTable, item1);
    insert_item_to_hash_table(hashTable, item2);

    ck_assert_int_eq(hashTable->itemsCount, 1);
    ck_assert_int_eq(hashTable->linksCount, 1);

    ck_assert_float_eq(calculate_load_factor(hashTable), 1.0);

    delete_hash_table(hashTable);
}
END_TEST

Suite* hash_table_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Hash table");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_hash_table);
    tcase_add_test(tc_core, test_create_hash_item);
    tcase_add_test(tc_core, test_is_hash_table_empty);
    tcase_add_test(tc_core, test_is_hash_table_full);
    tcase_add_test(tc_core, test_insert_item_to_hash_table);
    tcase_add_test(tc_core, test_remove_item_from_hash_table);
    tcase_add_test(tc_core, test_find_item_in_hash_table);
    tcase_add_test(tc_core, test_string_hash_function);
    tcase_add_test(tc_core, test_calculate_load_factor);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = hash_table_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
