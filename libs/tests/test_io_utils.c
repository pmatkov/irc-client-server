#include "../src/io_utils.h"
#include "../src/string_utils.h"
#include "../src/mock.h"

#include <check.h>
#include <unistd.h>

static void initialize_mock_objects(char *buffer, int size) {

    set_mock_buffer(buffer);
    set_mock_buffer_size(size);
    set_mock_fd(5);
}

START_TEST(test_create_pipe) {

    int pipeFd[2];

    create_pipe(pipeFd);

    ck_assert_int_ne(pipeFd[READ_PIPE], 0);
    ck_assert_int_ne(pipeFd[WRITE_PIPE], 0);

    close(pipeFd[READ_PIPE]);
    close(pipeFd[WRITE_PIPE]);

}
END_TEST

START_TEST(test_read_string) {

    char *string = "message";
    char buffer[MAX_CHARS + 1] = {'\0'};

    initialize_mock_objects(string, strlen(string));

    int bytesRead = read_string(get_mock_fd(), buffer, sizeof(buffer));

    ck_assert_int_eq(bytesRead, strlen(string));
    ck_assert_str_eq(string, buffer);

}
END_TEST

START_TEST(test_write_string) {

    char *string = "message";
    char buffer[MAX_CHARS + 1] = {'\0'};

    initialize_mock_objects(buffer, ARR_SIZE(buffer));

    int status = write_string(get_mock_fd(), string);

    ck_assert_int_eq(status, strlen(string));
    ck_assert_str_eq(string, buffer);
}
END_TEST

START_TEST(test_read_message) {

    char *string = "message\r\n";
    char buffer[MAX_CHARS + 1] = {'\0'};

    initialize_mock_objects(string, strlen(string));

    int readStatus = read_message(get_mock_fd(), buffer, sizeof(buffer));

    ck_assert_int_eq(readStatus, 1);
    ck_assert_str_eq(buffer, "message");

}
END_TEST

START_TEST(test_write_message) {

    char *string = "message";
    char buffer[MAX_CHARS + 1] = {'\0'};

    initialize_mock_objects(buffer, ARR_SIZE(buffer));

    int writeStatus = write_message(get_mock_fd(), string);

    ck_assert_int_eq(writeStatus, 1);
    ck_assert_str_eq(buffer, "message\r\n");
}
END_TEST

Suite* io_utils_suite(void) {
    Suite *s;
    TCase *tc_core, *tc_convert;

    s = suite_create("IO utils");
    tc_core = tcase_create("Core");
    tc_convert = tcase_create("Convert");

    // Add the test case to the test suite
    tcase_add_test(tc_convert, test_create_pipe);
    tcase_add_test(tc_convert, test_read_string);
    tcase_add_test(tc_convert, test_write_string);
    tcase_add_test(tc_convert, test_read_message);
    tcase_add_test(tc_convert, test_write_message);

    suite_add_tcase(s, tc_convert);
    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = io_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
