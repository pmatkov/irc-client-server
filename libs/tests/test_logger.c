#include "../src/priv_logger.h"

#include <check.h>

#define DEF_LOG_FREQUENCY 20

START_TEST(test_create_logger) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    ck_assert_ptr_ne(logger, NULL);
    ck_assert_int_eq(logger->logLevel, DEBUG);
    ck_assert_int_eq(logger->logFrequency, 1);
    ck_assert_int_eq(logger->stdoutEnabled, 1);
    ck_assert_int_eq(logger->capacity, DEF_LOG_FREQUENCY);
    ck_assert_int_eq(logger->currentCount, 0);
    ck_assert_int_eq(logger->totalCount, 0);

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

    ck_assert_int_eq(logger->currentCount, 0);
    ck_assert_int_eq(logger->totalCount, 2);

    delete_logger(logger);

}
END_TEST

START_TEST(test_log_error) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    ck_assert_ptr_ne(logger, NULL);

    log_error(NO_ERRCODE, "Error message", __func__, __FILE__, __LINE__, 0);
    log_error(ARG_ERROR, NULL, __func__, __FILE__, __LINE__, 0);
    ck_assert_int_eq(logger->currentCount, 0);
    ck_assert_int_eq(logger->totalCount, 2);

    delete_logger(logger);

}
END_TEST

START_TEST(test_set_streams) {

    Logger *logger = create_logger(NULL, "test", DEBUG);

    enable_stdout_logging(1);
    ck_assert_int_eq(logger->stdoutEnabled, 1);
    
    delete_logger(logger);
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
    tcase_add_test(tc_core, test_set_streams);

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
