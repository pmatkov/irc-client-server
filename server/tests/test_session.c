#include "../src/priv_session.h"
#include "../src/priv_user.h"
#include "../src/priv_channel.h"
#include "../src/priv_linked_list.h"

#include <check.h>

#define START_USERS 500
#define START_CHANNELS 50

#define MAX_USERS START_USERS * LOAD_FACTOR
#define MAX_CHANNELS START_CHANNELS * LOAD_FACTOR

#define MAX_CHANNELS_PER_USER 5
#define MAX_USERS_PER_CHANNEL 100

START_TEST(test_create_session) {

    Session *session = create_session();

    ck_assert_ptr_ne(session, NULL);
    ck_assert_ptr_ne(session->users, NULL);
    ck_assert_ptr_ne(session->channels, NULL);
    ck_assert_ptr_ne(session->userChannelsLL, NULL);
    ck_assert_ptr_ne(session->channelUsersLL, NULL);

    ck_assert_int_eq(session->usersCapacity, MAX_USERS);
    ck_assert_int_eq(session->usersCount, 0);
    ck_assert_int_eq(session->channelsCapacity, MAX_CHANNELS);
    ck_assert_int_eq(session->channelsCount, 0);

    delete_session(session);
}
END_TEST

START_TEST(test_add_user_to_hash_table) {

    Session *session = create_session();

    User *user = create_user("john", NULL, NULL, NULL, 0);

    add_user_to_hash_table(session, user);

    ck_assert_int_eq(session->usersCount, 1);

    delete_session(session);
}
END_TEST

START_TEST(test_remove_user_from_hash_table) {

    Session *session = create_session();

    User *user = create_user("john", NULL, NULL, NULL, 0);

    add_user_to_hash_table(session, user);
    remove_user_from_hash_table(session, user);

    ck_assert_int_eq(session->usersCount, 0);

    delete_session(session);
}
END_TEST

START_TEST(test_find_user_in_hash_table) {

    Session *session = create_session();

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    User *user2 = create_user("mark", NULL, NULL, NULL, 0);
    add_user_to_hash_table(session, user1);
    add_user_to_hash_table(session, user2);

    User *user = find_user_in_hash_table(session, "mark");

    ck_assert_ptr_eq(user, user2);
    ck_assert_str_eq(user2->nickname, "mark");

    delete_session(session);
}
END_TEST

START_TEST(test_change_user_in_hash_table) {

    Session *session = create_session();

    User *oldUser = create_user("john", NULL, NULL, NULL, 0);
    User *newUser = create_user("mark", NULL, NULL, NULL, 0);
    add_user_to_hash_table(session, oldUser);

    User *user = find_user_in_hash_table(session, "john");
    ck_assert_ptr_eq(user, oldUser);

    change_user_in_hash_table(session, oldUser, newUser);

    ck_assert_int_eq(session->usersCount, 1);

    user = find_user_in_hash_table(session, "mark");
    ck_assert_ptr_eq(user, newUser);
    user = find_user_in_hash_table(session, "john");
    ck_assert_ptr_eq(user, NULL);

    delete_session(session);
}
END_TEST

START_TEST(test_create_user_channels) {

    User *user = create_user("john", NULL, NULL, NULL, 0);
    UserChannels *userChannels = create_user_channels(user);

    ck_assert_ptr_ne(userChannels, NULL);
    ck_assert_ptr_ne(userChannels->channels, NULL);
    ck_assert_str_eq(userChannels->user->nickname, "john");
    ck_assert_int_eq(userChannels->capacity, MAX_CHANNELS_PER_USER);
    ck_assert_int_eq(userChannels->count, 0);

    delete_user_channels(userChannels);
    delete_user(user);
}
END_TEST

START_TEST(test_add_user_channels) {

    Session *session = create_session();

    User *user = create_user("john", NULL, NULL, NULL, 0);
    UserChannels *userChannels = create_user_channels(user);

    add_user_channels(session, userChannels);

    ck_assert_int_eq(session->userChannelsLL->count, 1);

    delete_user(user);
    delete_session(session);
}
END_TEST

START_TEST(test_remove_user_channels) {

    Session *session = create_session();

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    User *user2 = create_user("mark", NULL, NULL, NULL, 0);

    UserChannels *userChannels1 = create_user_channels(user1);
    UserChannels *userChannels2 = create_user_channels(user2);

    add_user_channels(session, userChannels1);
    add_user_channels(session, userChannels2);

    ck_assert_int_eq(session->userChannelsLL->count, 2);

    remove_user_channels(session, userChannels2);

    ck_assert_int_eq(session->userChannelsLL->count, 1);

    delete_user(user1);
    delete_user(user2);
    delete_session(session);
}
END_TEST


START_TEST(test_find_user_channels) {

    Session *session = create_session();

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    User *user2 = create_user("mark", NULL, NULL, NULL, 0);

    UserChannels *userChannels1 = create_user_channels(user1);
    UserChannels *userChannels2 = create_user_channels(user2);

    add_user_channels(session, userChannels1);
    add_user_channels(session, userChannels2);

    ck_assert_int_eq(session->userChannelsLL->count, 2);

    UserChannels *userChannels = find_user_channels(session, user2);

    ck_assert_ptr_eq(userChannels, userChannels2);

    delete_user(user1);
    delete_user(user2);
    delete_session(session);
}
END_TEST

START_TEST(test_add_channel_to_user_channels) {

    User *user = create_user("john", NULL, NULL, NULL, 0);
    Channel *channel = create_channel("general", TEMPORARY, MAX_USERS_PER_CHANNEL);

    UserChannels *userChannels = create_user_channels(user);
    add_channel_to_user_channels(userChannels, channel);    

    ck_assert_int_eq(userChannels->count, 1);

    delete_user_channels(userChannels);
    delete_user(user);
}
END_TEST

START_TEST(test_find_channel_in_user_channels) {

    User *user = create_user("john", NULL, NULL, NULL, 0);
    Channel *channel1 = create_channel("general", TEMPORARY, MAX_USERS_PER_CHANNEL);
    Channel *channel2 = create_channel("linux", TEMPORARY, MAX_USERS_PER_CHANNEL);

    UserChannels *userChannels = create_user_channels(user);
    add_channel_to_user_channels(userChannels, channel1);    
    add_channel_to_user_channels(userChannels, channel2);   

    ck_assert_int_eq(userChannels->count, 2);
    Channel *channel = find_channel_in_user_channels(userChannels, channel2);

    ck_assert_ptr_eq(channel, channel2);

    delete_user_channels(userChannels);
    delete_user(user);
}
END_TEST

START_TEST(test_change_user_in_user_channels) {

    User *oldUser = create_user("john", NULL, NULL, NULL, 0);
    User *newUser = create_user("mark", NULL, NULL, NULL, 0);
    UserChannels *userChannels = create_user_channels(oldUser);    

    ck_assert_int_eq(userChannels->user->nickname, "john");

    change_user_in_user_channels(userChannels, newUser);

    ck_assert_int_eq(userChannels->user->nickname, "mark");

    delete_user_channels(userChannels);
    delete_user(oldUser);
    delete_user(newUser);
}
END_TEST

START_TEST(test_are_user_channels_equal) {

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    User *user2 = create_user("mark", NULL, NULL, NULL, 0);

    UserChannels *userChannels1 = create_user_channels(user1);
    UserChannels *userChannels2 = create_user_channels(user2);

    int equal = are_user_channels_equal(userChannels1, userChannels2);

    ck_assert_int_eq(equal, 0);

    delete_user_channels(userChannels1);
    delete_user_channels(userChannels2);
    delete_user(user1);
    delete_user(user2);
}
END_TEST


Suite* session_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Session");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_session);
    tcase_add_test(tc_core, test_add_user_to_hash_table);
    tcase_add_test(tc_core, test_remove_user_from_hash_table);
    tcase_add_test(tc_core, test_find_user_in_hash_table);
    tcase_add_test(tc_core, test_create_user_channels);
    tcase_add_test(tc_core, test_add_user_channels);
    tcase_add_test(tc_core, test_remove_user_channels);
    tcase_add_test(tc_core, test_find_user_channels);
    tcase_add_test(tc_core, test_add_channel_to_user_channels);
    tcase_add_test(tc_core, test_find_channel_in_user_channels);
    tcase_add_test(tc_core, test_change_user_in_user_channels);
    tcase_add_test(tc_core, test_are_user_channels_equal);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = session_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
