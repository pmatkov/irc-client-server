#include "../src/priv_command_handler.h"
#include "../../shared/src/priv_command.h"

#include <check.h>

#define MAX_CHARS 512

START_TEST(test_create_notice) {

    char notice[MAX_CHARS + 1] = {'\0'};

    create_notice(notice, MAX_CHARS - strlen("QUIT") - 1,  (const char *[]){"see", "you", "tomorrow again"}, 3);

    ck_assert_str_eq(notice, ":see you tomorrow again");
}
END_TEST

Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_notice);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = command_handler_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
