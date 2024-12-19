#include "../src/priv_message.h"

#include <check.h>

START_TEST(test_create_message) {

    Message *message = create_message("/JOIN #general", "", MSG_COMMAND, NORMAL_PRIORTY);

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message->content, "/JOIN #general");
    ck_assert_str_eq(message->separator, "");
    ck_assert_int_eq(message->messageType, MSG_COMMAND);
    ck_assert_int_eq(message->messagePriority, NORMAL_PRIORTY);

    delete_message(message);

}
END_TEST

START_TEST(test_set_message_char) {

    Message *message = create_message("rock", "", MSG_STANDARD, NORMAL_PRIORTY);

    set_message_char(message, 's', 0);
    ck_assert_str_eq(message->content, "sock");

    delete_message(message);

}
END_TEST

START_TEST(test_get_set_message_data) {

    Message *message = create_message("blueberry", "", MSG_STANDARD, NORMAL_PRIORTY);

    ck_assert_str_eq(get_message_content(message), "blueberry");

    set_message_content(message, "apple");
    ck_assert_str_eq(get_message_content(message), "apple");

    ck_assert_int_eq(get_message_type(message), MSG_STANDARD);
    ck_assert_int_eq(get_message_priority(message), NORMAL_PRIORTY);

    delete_message(message);

}
END_TEST

START_TEST(test_serialize_deserialize_message) {

    Message *message = create_message("blueberry", "", MSG_STANDARD, NORMAL_PRIORTY);

    char buffer[MAX_CHARS * 2 + 1] = {'\0'};

    Message result;

    serialize_message(buffer, ARRAY_SIZE(buffer), message);
    deserialize_message(buffer, ARRAY_SIZE(buffer), &result);

    ck_assert_str_eq(get_message_content(message), "blueberry");
    ck_assert_int_eq(get_message_type(message), MSG_STANDARD);
    ck_assert_int_eq(get_message_priority(message), NORMAL_PRIORTY);

    delete_message(message);

}
END_TEST


Suite* message_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Message");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_message);
    tcase_add_test(tc_core, test_set_message_char);
    tcase_add_test(tc_core, test_get_set_message_data);
    tcase_add_test(tc_core, test_serialize_deserialize_message);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = message_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
