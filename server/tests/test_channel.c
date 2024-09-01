#include "../src/test_channel.h"

#include <check.h>

START_TEST(test_create_channel_list) {

    ChannelList *cl = create_channel_list();

    ck_assert_ptr_ne(cl, NULL);
    ck_assert_ptr_eq(cl->head, NULL);
    ck_assert_int_eq(cl->channelCount, 0);

    delete_channel_list(cl);
}
END_TEST


START_TEST(test_is_channel_list_empty) {

    ChannelList *cl = create_channel_list();

    add_channel(cl, "general", PERSISTENT);
    
    ck_assert_int_eq(is_channel_list_empty(cl), 0);

    delete_channel_list(cl);
}
END_TEST


START_TEST(test_is_channel_list_full) {

    ChannelList *cl = create_channel_list();
    
    ck_assert_int_eq(is_channel_list_full(cl), 0);

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_create_channel) {

    Channel *channel = create_channel("general", PERSISTENT);

    ck_assert_ptr_ne(channel, NULL);
    ck_assert_str_eq(channel->name, "general");
    ck_assert_int_eq(channel->channelType, PERSISTENT);
    ck_assert_int_eq(channel->usersCount, 0);
    
    delete_channel(channel);
}
END_TEST

START_TEST(test_add_channel) {

    ChannelList *cl = create_channel_list();

    Channel *channel = add_channel(cl, "general", PERSISTENT);

    ck_assert_ptr_ne(channel, NULL);
    ck_assert_int_eq(cl->channelCount, 1);

    channel = add_channel(cl, "networking", PERSISTENT);

    ck_assert_ptr_ne(channel, NULL);
    ck_assert_int_eq(cl->channelCount, 2);

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_remove_channel) {

    ChannelList *cl = create_channel_list();

    int removed = remove_channel(cl, "general");
    ck_assert_int_eq(removed, 0);

    Channel *ch = add_channel(cl, "general", PERSISTENT);
    ck_assert_ptr_ne(ch, NULL);
    ck_assert_int_eq(cl->channelCount, 1);

    removed = remove_channel(cl, "general");
    ck_assert_int_eq(removed, 1);
    ck_assert_int_eq(cl->channelCount, 0);

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_lookup_channel) {

    ChannelList *cl = create_channel_list();

    add_channel(cl, "general", PERSISTENT);
    add_channel(cl, "networking", PERSISTENT);

    Channel *channel = lookup_channel(cl, "networking");

    ck_assert_ptr_ne(channel, NULL);
    ck_assert_str_eq(channel->name, "networking");

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_is_channel_empty) {

    ChannelList *cl = create_channel_list();

    Channel *channel = add_channel(cl, "general", PERSISTENT);
    
    ck_assert_ptr_ne(channel, NULL);
    ck_assert_int_eq(is_channel_empty(channel), 1);

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_is_channel_full) {

    ChannelList *cl = create_channel_list();

    Channel *channel = add_channel(cl, "general", PERSISTENT);
    
    ck_assert_ptr_ne(channel, NULL);
    ck_assert_int_eq(is_channel_full(channel), 0);

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_add_user_to_channel) {

    ChannelList *cl = create_channel_list();

    Channel *channel = add_channel(cl, "general", PERSISTENT);

    User user;
    user.messageQueue = NULL;
    user.socketFd = 3;
    strcpy(user.nickname, "john");
    user.next = NULL;

    int added = add_user_to_channel(channel, &user);

    ck_assert_int_eq(added, 1);
    ck_assert_int_eq(channel->usersCount, 1);

    delete_channel_list(cl);

}
END_TEST

START_TEST(test_remove_user_from_channel) {

    ChannelList *cl = create_channel_list();

    Channel *channel = add_channel(cl, "general", PERSISTENT);

    User user;
    user.messageQueue = NULL;
    user.socketFd = 3;
    strcpy(user.nickname, "john");
    user.next = NULL;

    int added = add_user_to_channel(channel, &user);

    ck_assert_int_eq(added, 1);
    ck_assert_int_eq(channel->usersCount, 1);

    int removed = remove_user_from_channel(channel, "john");

    ck_assert_int_eq(removed, 1);
    ck_assert_int_eq(channel->usersCount, 0);

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_lookup_user_in_channel) {

    ChannelList *cl = create_channel_list();
    Channel *channel = add_channel(cl, "general", PERSISTENT);

    User user1, user2;

    user1.messageQueue = NULL;
    user1.socketFd = 3;
    strcpy(user1.nickname, "john");
    user1.next = NULL;

    user2.messageQueue = NULL;
    user2.socketFd = 4;
    strcpy(user2.nickname, "steve");
    user2.next = NULL;

    add_user_to_channel(channel, &user1);
    add_user_to_channel(channel, &user2);

    ck_assert_int_eq(channel->usersCount, 2);

    User *user = lookup_user_in_channel(channel, "steve");

    ck_assert_ptr_ne(user, NULL);
    ck_assert_str_eq(user->nickname, "steve");

    delete_channel_list(cl);
}
END_TEST

START_TEST(test_is_valid_channel_type) {

    int valid = is_valid_channel_type(PERSISTENT);
    ck_assert_int_eq(valid, 1);

    valid = is_valid_channel_type(4);
    ck_assert_int_eq(valid, 0);
}
END_TEST


Suite* channel_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("channel");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_channel_list);
    tcase_add_test(tc_core, test_is_channel_list_empty);
    tcase_add_test(tc_core, test_is_channel_list_full);

    tcase_add_test(tc_core, test_create_channel);
    tcase_add_test(tc_core, test_add_channel);
    tcase_add_test(tc_core, test_remove_channel);
    tcase_add_test(tc_core, test_lookup_channel);
    tcase_add_test(tc_core, test_is_channel_empty);
    tcase_add_test(tc_core, test_is_channel_full);

    tcase_add_test(tc_core, test_add_user_to_channel);
    tcase_add_test(tc_core, test_remove_user_from_channel);
    tcase_add_test(tc_core, test_lookup_user_in_channel);

    tcase_add_test(tc_core, test_is_valid_channel_type);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = channel_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif