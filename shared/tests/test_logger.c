#include "../src/priv_logger.h"

#include <check.h>

#define MAX_LINES 25

START_TEST(test_create_logger) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    ck_assert_ptr_ne(logger, NULL);
    ck_assert_int_eq(logger->allocatedLines, MAX_LINES);
    ck_assert_int_eq(logger->usedLines, 0);

    delete_logger(logger);
}
END_TEST

START_TEST(test_open_log_file) {

    FILE *fp = open_log_file(NULL, "test");

    ck_assert_ptr_ne(fp, NULL);

}
END_TEST

START_TEST(test_log_message) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    ck_assert_ptr_ne(logger, NULL);

    log_message(INFO, "Test message", __func__, __FILE__, __LINE__);
    log_message(INFO, "Test message from: %s", __func__, __FILE__, __LINE__, "john");

    ck_assert_int_eq(logger->usedLines, 2);

    delete_logger(logger);

}
END_TEST

START_TEST(test_log_error) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    ck_assert_ptr_ne(logger, NULL);

    log_error("Error message", NO_ERRCODE, __func__, __FILE__, __LINE__, 0);
    log_error(NULL, ARG_ERROR, __func__, __FILE__, __LINE__, 0);
    ck_assert_int_eq(logger->usedLines, 2);

    delete_logger(logger);

}
END_TEST

START_TEST(test_set_stdout_allowed) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    set_stdout_allowed(1);

    ck_assert_int_eq(logger->stdoutAllowed, 1);
    
    delete_logger(logger);
}
END_TEST

START_TEST(test_is_valid_log_level) {

    ck_assert_int_eq(is_valid_log_level(DEBUG), 1);
    ck_assert_int_eq(is_valid_log_level(UNKNOWN_LOGLEVEL), 0);
    
}
END_TEST

START_TEST(test_log_level_to_string) {

    ck_assert_str_eq(log_level_to_string(DEBUG), "debug");
    ck_assert_ptr_eq(log_level_to_string(10), NULL);

}
END_TEST

START_TEST(test_string_to_log_level) {

    ck_assert_int_eq(string_to_log_level("debug"), DEBUG);
    ck_assert_int_eq(string_to_log_level("level"), UNKNOWN_LOGLEVEL);

}
END_TEST

Suite* logger_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Logger");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_logger);
    tcase_add_test(tc_core, test_open_log_file);
    tcase_add_test(tc_core, test_log_message);
    tcase_add_test(tc_core, test_log_error);
    tcase_add_test(tc_core, test_set_stdout_allowed);
    tcase_add_test(tc_core, test_is_valid_log_level);
    tcase_add_test(tc_core, test_log_level_to_string);
    tcase_add_test(tc_core, test_string_to_log_level);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = logger_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
