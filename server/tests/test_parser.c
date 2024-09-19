#include "../src/priv_command_handler.h"
#include "../../shared/src/response_code.h"

#include <check.h>

#define MAX_CHARS 512

START_TEST(test_create_notice) {

    char notice[MAX_CHARS + 1] = {'\0'};

    create_fwd_message(notice, MAX_CHARS, "jdoe!john@client.example.com", "NICK", "jmax");

    ck_assert_str_eq(notice, ":jdoe!john@client.example.com NICK jmax");

}
END_TEST

START_TEST(test_create_message) {

    char message[MAX_CHARS + 1] = {'\0'};

    create_response(message, MAX_CHARS, "irc.example.com", "john", ERR_NICKNAMEINUSE);

    ck_assert_str_eq(message, ":irc.example.com 433 john :Nickname is already in use");

}
END_TEST

START_TEST(test_create_client_info) {

    char clientInfo[MAX_CHARS + 1] = {'\0'};

    create_client_info(clientInfo, MAX_CHARS, "jdoe", "john", "client.example.com");

    ck_assert_str_eq(clientInfo, "jdoe!john@client.example.com");

}
END_TEST

Suite* parser_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Parser");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_notice);
    tcase_add_test(tc_core, test_create_message);
    tcase_add_test(tc_core, test_create_client_info);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = parser_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif