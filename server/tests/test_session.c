#ifndef TEST
#define TEST
#endif

#include "../src/priv_session.h"
#include "../src/priv_user.h"
#include "../src/priv_channel.h"
#include "../../libs/src/priv_hash_table.h"
#include "../../libs/src/priv_linked_list.h"

#include <check.h>

static int count_removable_channels(Session *session, UserChannels *userChannels) {

    LinkedList *channels = get_channels_from_user_channels(userChannels);
    reset_iterator(channels);

    int removableChannels = 0;
    Node *node = NULL;

    while ((node = iterator_next(channels)) != NULL) {

        Channel *channel = get_data(node);
        ChannelUsers *channelUsers = find_channel_users(session, channel);

        if (channelUsers != NULL && get_channel_users_count(channelUsers) == 1) {
            removableChannels++;
        }
    }
    return removableChannels;
}

START_TEST(test_create_session) {

    Session *session = create_session();

    ck_assert_ptr_ne(session, NULL);
    ck_assert_ptr_ne(session->readyList, NULL);
    ck_assert_ptr_ne(session->users, NULL);
    ck_assert_ptr_ne(session->channels, NULL);
    ck_assert_ptr_ne(session->userChannelsLL, NULL);
    ck_assert_ptr_ne(session->channelUsersLL, NULL);

    delete_session(session);
}
END_TEST

START_TEST(test_add_user_to_hash_table) {

    Session *session = create_session();

    User *user = create_user(0, "john", NULL, NULL, NULL);

    add_user_to_hash_table(session, user);

    ck_assert_int_eq(get_total_items(session->users), 1);

    delete_session(session);
}
END_TEST

START_TEST(test_remove_user_from_hash_table) {

    Session *session = create_session();

    User *user = create_user(0, "john", NULL, NULL, NULL);

    add_user_to_hash_table(session, user);
    remove_user_from_hash_table(session, user);

    ck_assert_int_eq(get_total_items(session->users), 0);

    delete_session(session);
}
END_TEST

START_TEST(test_find_user_in_hash_table) {

    Session *session = create_session();

    User *user1 = create_user(0, "john", NULL, NULL, NULL);
    User *user2 = create_user(0, "mark", NULL, NULL, NULL);
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

    User *oldUser = create_user(0, "john", NULL, NULL, NULL);
    User *newUser = create_user(0, "mark", NULL, NULL, NULL);
    add_user_to_hash_table(session, oldUser);

    User *user = find_user_in_hash_table(session, "john");
    ck_assert_ptr_eq(user, oldUser);

    change_user_in_hash_table(session, oldUser, newUser);

    ck_assert_int_eq(get_total_items(session->users), 1);

    user = find_user_in_hash_table(session, "mark");
    ck_assert_ptr_eq(user, newUser);
    user = find_user_in_hash_table(session, "john");
    ck_assert_ptr_eq(user, NULL);

    delete_session(session);
}
END_TEST

START_TEST(test_create_ready_list) {

    ReadyList *readyList = create_ready_list();

    ck_assert_ptr_ne(readyList->readyUsers, NULL);
    ck_assert_ptr_ne(readyList->readyChannels, NULL);
    ck_assert_int_eq(readyList->readyUsers->count, 0);
    ck_assert_int_eq(readyList->readyUsers->count, 0);

    delete_ready_list(readyList);
}
END_TEST

START_TEST(test_add_remove_user_ready_list) {

    ReadyList *readyList = create_ready_list();
    User *user = create_user(0, "john", NULL, NULL, NULL);

    add_user_to_ready_list(user, readyList);
    ck_assert_int_eq(readyList->readyUsers->count, 1);

    add_user_to_ready_list(user, readyList);
    ck_assert_int_eq(readyList->readyUsers->count, 1);

    remove_user_from_ready_list(readyList->readyUsers, user);
    ck_assert_int_eq(readyList->readyUsers->count, 0);

    delete_user(user);
    delete_ready_list(readyList);
}
END_TEST

START_TEST(test_add_remove_channel_ready_list) {

    ReadyList *readyList = create_ready_list();
    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);

    add_channel_to_ready_list(channel, readyList);
    ck_assert_int_eq(readyList->readyChannels->count, 1);

    add_channel_to_ready_list(channel, readyList);
    ck_assert_int_eq(readyList->readyChannels->count, 1);

    remove_channel_from_ready_list(readyList->readyChannels, channel);
    ck_assert_int_eq(readyList->readyChannels->count, 0);

    delete_channel(channel);
    delete_ready_list(readyList);
}
END_TEST

START_TEST(test_create_user_channels) {

    User *user = create_user(0, "john", NULL, NULL, NULL);
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

    User *user = create_user(0, "john", NULL, NULL, NULL);
    UserChannels *userChannels = create_user_channels(user);

    add_user_channels(session, userChannels);

    ck_assert_int_eq(session->userChannelsLL->count, 1);

    delete_user(user);
    delete_session(session);
}
END_TEST

START_TEST(test_remove_user_channels) {

    Session *session = create_session();

    User *user1 = create_user(0, "john", NULL, NULL, NULL);
    User *user2 = create_user(0, "mark", NULL, NULL, NULL);

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

    User *user1 = create_user(0, "john", NULL, NULL, NULL);
    User *user2 = create_user(0, "mark", NULL, NULL, NULL);

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

    User *user = create_user(0, "john", NULL, NULL, NULL);
    UserChannels *userChannels = create_user_channels(user);

    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel);    

    ck_assert_int_eq(userChannels->count, 1);

    delete_user_channels(userChannels);

    delete_channel(channel);
    delete_user(user);
}
END_TEST

START_TEST(test_find_channel_in_user_channels) {

    User *user = create_user(0, "john", NULL, NULL, NULL);
    UserChannels *userChannels = create_user_channels(user);

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    Channel *channel2 = create_channel("#linux", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);

    add_channel_to_user_channels(userChannels, channel1);    
    add_channel_to_user_channels(userChannels, channel2);   

    ck_assert_int_eq(userChannels->count, 2);
    Channel *channel = find_channel_in_user_channels(userChannels, channel2);

    ck_assert_ptr_eq(channel, channel2);

    delete_user_channels(userChannels);

    delete_channel(channel1);
    delete_channel(channel2);
    delete_user(user);
}
END_TEST

START_TEST(test_remove_channel_in_user_channels) {

    User *user = create_user(0, "john", NULL, NULL, NULL);
    UserChannels *userChannels = create_user_channels(user);

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    Channel *channel2 = create_channel("#linux", NULL,  TEMPORARY, MAX_USERS_PER_CHANNEL);

    add_channel_to_user_channels(userChannels, channel1);    
    add_channel_to_user_channels(userChannels, channel2);  

    ck_assert_int_eq(userChannels->count, 2);
    remove_channel_in_user_channels(userChannels, channel2);

    ck_assert_int_eq(userChannels->count, 1);

    delete_user_channels(userChannels);

    delete_channel(channel1);
    delete_channel(channel2);
    delete_user(user);
    
}
END_TEST

START_TEST(test_change_user_in_user_channels) {

    User *oldUser = create_user(0, "john", NULL, NULL, NULL);
    User *newUser = create_user(0, "mark", NULL, NULL, NULL);
    UserChannels *userChannels = create_user_channels(oldUser);

    ck_assert_str_eq(userChannels->user->nickname, "john");

    change_user_in_user_channels(userChannels, newUser);

    ck_assert_str_eq(userChannels->user->nickname, "mark");

    delete_user_channels(userChannels);
    delete_user(oldUser);
    delete_user(newUser);

}
END_TEST

START_TEST(test_find_removable_channels) {

    Session *session = create_session();

    User *user1 = create_user(0, "john", NULL, NULL, NULL);
    User *user2 = create_user(0, "mark", NULL, NULL, NULL);

    UserChannels *userChannels1 = create_user_channels(user1);
    UserChannels *userChannels2 = create_user_channels(user2);

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    Channel *channel2 = create_channel("#linux", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    
    add_channel_to_user_channels(userChannels1, channel1);  
    add_channel_to_user_channels(userChannels1, channel2); 
    add_channel_to_user_channels(userChannels2, channel1);

    ck_assert_int_eq(userChannels1->count, 2);
    ck_assert_int_eq(userChannels2->count, 1);

    ChannelUsers *channelUsers1 = create_channel_users(channel1);
    ChannelUsers *channelUsers2 = create_channel_users(channel2);

    add_channel_users(session, channelUsers1);
    add_channel_users(session, channelUsers2);

    add_user_to_channel_users(channelUsers1, user1);  
    add_user_to_channel_users(channelUsers1, user2); 
    add_user_to_channel_users(channelUsers2, user1);

    ck_assert_int_eq(channelUsers1->count, 2);
    ck_assert_int_eq(channelUsers2->count, 1);

    LinkedList *channels = get_channels_from_user_channels(userChannels1);
    reset_iterator(channels);

    int removableChannels1 = count_removable_channels(session, userChannels1);
    ck_assert_int_eq(removableChannels1, 1);

    int removableChannels2 = count_removable_channels(session, userChannels2);
    ck_assert_int_eq(removableChannels2, 0);

    delete_user_channels(userChannels1);
    delete_user_channels(userChannels2);

    delete_channel(channel1);
    delete_channel(channel2);
    delete_user(user1);
    delete_user(user2);

    delete_session(session);
}
END_TEST

START_TEST(test_are_user_channels_equal) {

    User *user1 = create_user(0, "john", NULL, NULL, NULL);
    User *user2 = create_user(0, "mark", NULL, NULL, NULL);

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
    tcase_add_test(tc_core, test_change_user_in_hash_table);    
    tcase_add_test(tc_core, test_create_ready_list);
    tcase_add_test(tc_core, test_add_remove_user_ready_list);
    tcase_add_test(tc_core, test_add_remove_channel_ready_list);
    tcase_add_test(tc_core, test_create_user_channels);
    tcase_add_test(tc_core, test_add_user_channels);
    tcase_add_test(tc_core, test_remove_user_channels);
    tcase_add_test(tc_core, test_find_user_channels);
    tcase_add_test(tc_core, test_add_channel_to_user_channels);
    tcase_add_test(tc_core, test_find_channel_in_user_channels);
    tcase_add_test(tc_core, test_remove_channel_in_user_channels);
    tcase_add_test(tc_core, test_change_user_in_user_channels);
    tcase_add_test(tc_core, test_find_removable_channels);
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
