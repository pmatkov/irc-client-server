#include "../src/mock.h"

#include <check.h>

START_TEST(test_mock_read) {

    char input[] = "This is a unit test";
    char output[30] = {'\0'};
    mockBuffP = input;
    mockLen = strlen(input);
    mockFd = 1; 

    int bytesRead = mock_read(1, output, mockLen);

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(bytesRead, mockLen);
}
END_TEST

START_TEST(test_mock_write) {

    char input[] = "This is a unit test";
    char output[30] = {'\0'};
    mockBuffP = output;
    mockLen = strlen(input);
    mockFd = 1; 

    int bytesWritten = mock_write(1, input, mockLen);

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(bytesWritten, mockLen);
}
END_TEST

START_TEST(test_mock_close) {

    mockFd = 1; 

    int status = mock_close(1);

    ck_assert_int_eq(status, 0);
}
END_TEST

Suite* mocks_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Mocks");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_mock_read);
    tcase_add_test(tc_core, test_mock_write);
    tcase_add_test(tc_core, test_mock_close);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = mocks_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
