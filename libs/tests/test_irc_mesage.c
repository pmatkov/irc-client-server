#include "../src/irc_message.h"
#include "../../libs/src/common.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/response_code.h"

#include <check.h>

static void create_prefix(char *buffer, int size, void *arg) {

    safe_copy(buffer, size, arg);
}

START_TEST(test_create_irc_message) {

    char message[MAX_CHARS + 1] = {'\0'};

    const char *code = get_response_code(ERR_NICKNAMEINUSE);
    create_irc_message(message, MAX_CHARS, &(IRCMessage){{code, "john"}, {get_response_message(code)}, 1, create_prefix, "irc.example.com"});

    ck_assert_str_eq(message, ":irc.example.com 433 john :Nickname is already in use");

}
END_TEST

Suite* irc_message_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("IRC message");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_irc_message);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = irc_message_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
