#include "../src/priv_channel.h"
#include "../../libs/src/priv_queue.h"
#include "../../libs/src/priv_message.h"

#include <check.h>

START_TEST(test_create_channel) {

    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);

    ck_assert_ptr_ne(channel, NULL);
    ck_assert_str_eq(channel->name, "#general");
    ck_assert_int_eq(channel->channelType, TEMPORARY);

    delete_channel(channel);
}
END_TEST

START_TEST(test_enqueue_dequeue_channel) {

    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);

    enqueue_to_channel_queue(channel, "message");
    ck_assert_int_eq(channel->outQueue->count, 1);

    RegMessage *message = dequeue_from_channel_queue(channel);
    ck_assert_int_eq(channel->outQueue->count, 0);

    ck_assert_str_eq(get_reg_message_content(message), "message");

    delete_channel(channel);
}
END_TEST

START_TEST(test_are_channels_equal) {

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    Channel *channel2 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);

    int equal = are_channels_equal(channel1, channel2);
    ck_assert_int_eq(equal, 1);

    delete_channel(channel1);
    delete_channel(channel2);
}
END_TEST

START_TEST(test_get_channel_data) {

    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);

    ChannelType type = get_channel_type(channel);
    ck_assert_int_eq(type, TEMPORARY);

    const char *channelName = get_channel_name(channel);
    ck_assert_str_eq(channelName, "#general");

    Queue *queue = get_channel_queue(channel);
    ck_assert_ptr_ne(queue, NULL);

    delete_channel(channel);
}
END_TEST

Suite* channel_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Channel");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_channel);
    tcase_add_test(tc_core, test_enqueue_dequeue_channel);
    tcase_add_test(tc_core, test_are_channels_equal);
    tcase_add_test(tc_core, test_get_channel_data);
    
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