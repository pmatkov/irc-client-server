 #include "../src/priv_message.h"

#include <check.h>

START_TEST(test_create_reg_message) {

    RegMessage *message = create_reg_message("message");

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message->content, "message");

    delete_message(message);

}
END_TEST

START_TEST(test_create_ext_message) {

    ExtMessage *message = create_ext_message("john", "mark", "message");

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message->sender, "john"); 
    ck_assert_str_eq(message->recipient, "mark"); 
    ck_assert_str_eq(message->content, "message");

    delete_message(message);

}
END_TEST

START_TEST(test_get_char_from_message) {

    RegMessage *message = create_reg_message("message");

    char ch = get_char_from_message(message, 1, get_reg_message_content);
    ck_assert_int_eq(ch, 'e');

    ch = get_char_from_message(message, strlen(message->content), get_reg_message_content);
    ck_assert_int_eq(ch, '\0');

    delete_message(message);
}
END_TEST

START_TEST(test_set_char_in_message) {

    RegMessage *message = create_reg_message("message");

    set_char_in_message(message, '1', strlen(message->content), get_reg_message_content);

    ck_assert_str_eq(message->content, "message1");

    delete_message(message);
}
END_TEST

START_TEST(test_get_message_content_recipient) {

    ExtMessage *message = create_ext_message("", "john", "message");

    ck_assert_str_eq(get_ext_message_content(message), "message");
    ck_assert_str_eq(get_ext_message_recipient(message), "john");

    delete_message(message);
}
END_TEST

Suite* message_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Message");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_reg_message);
    tcase_add_test(tc_core, test_create_ext_message);
    tcase_add_test(tc_core, test_get_char_from_message);
    tcase_add_test(tc_core, test_set_char_in_message);
    tcase_add_test(tc_core, test_get_message_content_recipient);

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
