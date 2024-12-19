#include "../src/priv_user.h"
#include "../../libs/src/common.h"

#include <check.h>

START_TEST(test_create_user) {

    User *user = create_user(0, "john", NULL, NULL, NULL);

    ck_assert_ptr_ne(user, NULL);
    ck_assert_int_eq(user->fd, 0);
    ck_assert_str_eq(user->nickname, "john");
    ck_assert_str_eq(user->username, "");
    ck_assert_str_eq(user->hostname, "");
    ck_assert_str_eq(user->realname, "");
    ck_assert_ptr_ne(user->outQueue, NULL);

    delete_user(user);
}
END_TEST

START_TEST(test_copy_user) {

    User *user = create_user(0, "john", NULL, NULL, NULL);
    User *userCopy = copy_user(user);

    ck_assert_ptr_ne(user, NULL);
    ck_assert_ptr_ne(user, userCopy);

    ck_assert_int_eq(userCopy->fd, 0);
    ck_assert_str_eq(userCopy->nickname, "john");
    ck_assert_str_eq(userCopy->username, "");
    ck_assert_ptr_ne(userCopy->outQueue, NULL);

    delete_user(userCopy);
    delete_user(user);
}
END_TEST

START_TEST(test_enqueue_dequeue_user) {

    User *user = create_user(0, "mark", NULL, NULL, NULL);

    enqueue_to_user_queue(user, "message");

    ck_assert_int_eq(user->outQueue->count, 1);

    char *message = dequeue_from_user_queue(user);

    ck_assert_int_eq(user->outQueue->count, 0);
    ck_assert_str_eq(message, "message");

    delete_user(user);
}
END_TEST

START_TEST(test_are_users_equal) {

    User *user1 = create_user(0, "mark", "mmarcus", "irc1.client.com", "marky mark");
    User *user2 = create_user(0, "mark", "mmark", "irc1.client.com", "marky mark");
    User *user3 = create_user(0, "john", "jjones", "irc2.client.com", "john jones");
 
    int equal = are_users_equal(user1, user2);
    ck_assert_int_eq(equal, 1);

    equal = are_users_equal(user1, user3);
    ck_assert_int_eq(equal, 0);

    delete_user(user1);
    delete_user(user2);
    delete_user(user3);
}
END_TEST

START_TEST(test_add_nickname_to_list) {

    User *user = create_user(0, "john", NULL, NULL, NULL);

    struct {
        char buffer[MAX_CHARS + 1];
    } data = {{'\0'}};

    add_nickname_to_list(user, &data);

    ck_assert_str_eq(data.buffer, "john");

    delete_user(user);

}
END_TEST

START_TEST(test_create_user_info) {

    User *user = create_user(0, "jdoe", "john", "irc.client.com", NULL);

    char userInfo[MAX_CHARS + 1] = {'\0'};

    create_user_info(userInfo, MAX_CHARS, user);

    ck_assert_str_eq(userInfo, "jdoe!john@irc.client.com");

    delete_user(user);

}
END_TEST

START_TEST(test_get_user_data) {

    User *user = create_user(0, "john", "jjones", "client.irc.com", "John Jones");

    ck_assert_str_eq(get_user_nickname(user), "john");
    ck_assert_str_eq(get_user_username(user), "jjones");
    ck_assert_str_eq(get_user_hostname(user), "client.irc.com");
    ck_assert_str_eq(get_user_realname(user), "John Jones");
    ck_assert_ptr_ne(get_user_queue(user), NULL);

    delete_user(user);

}
END_TEST

Suite* user_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("User");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_user);
    tcase_add_test(tc_core, test_copy_user);
    tcase_add_test(tc_core, test_enqueue_dequeue_user);
    tcase_add_test(tc_core, test_are_users_equal);
    tcase_add_test(tc_core, test_add_nickname_to_list);
    tcase_add_test(tc_core, test_create_user_info);
    tcase_add_test(tc_core, test_get_user_data);
    
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