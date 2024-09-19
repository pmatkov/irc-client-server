#include "../src/priv_user.h"
#include "../../shared/src/priv_queue.h"
#include "../../shared/src/priv_message.h"

#include <check.h>

#define MAX_QUEUE_LEN 20

START_TEST(test_create_user) {

    User *user = create_user("john", NULL, NULL, NULL, 0);

    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user->nickname, "john");
    ck_assert_str_eq(user->username, "");
    ck_assert_int_eq(user->fd, 0);
    ck_assert_ptr_ne(user->outQueue, NULL);

    delete_user(user);
}
END_TEST

START_TEST(test_add_message_to_user_queue) {

    User *user = create_user("john", NULL, NULL, NULL, 0);

    add_message_to_user_queue(user, &(RegMessage){"message"});

    ck_assert_int_eq(user->outQueue->capacity, MAX_QUEUE_LEN);
    ck_assert_int_eq(user->outQueue->count, 1);

    delete_user(user);
}
END_TEST

START_TEST(test_set_user_data) {

    User *user = create_user("john", NULL, NULL, NULL, 0);
    set_user_data(user, "jjones", "irc.client.com", "john jones");

    ck_assert_str_eq(user->username, "jjones");
    ck_assert_str_eq(user->hostname, "irc.client.com");
    ck_assert_str_eq(user->realname, "john jones");

    delete_user(user);
}
END_TEST

START_TEST(test_are_users_equal) {

    User *user1 = create_user("john", "marotti", NULL, NULL, 0);
    User *user2 = create_user("john", "jones", NULL, NULL, 0);

    int equal = are_users_equal(user1, user2);
    ck_assert_int_eq(equal, 0);

    delete_user(user1);
    delete_user(user2);
}
END_TEST

Suite* user_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("User");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_user);
    tcase_add_test(tc_core, test_add_message_to_user_queue);
    tcase_add_test(tc_core, test_set_user_data);
    tcase_add_test(tc_core, test_are_users_equal);
    
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