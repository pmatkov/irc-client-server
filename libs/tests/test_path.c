#include "../src/path.h"

#include <check.h>

#define MAX_PATH 64

START_TEST(test_is_dir) {

    int status = is_dir("log");

    ck_assert_int_eq(status, 1);

}
END_TEST

START_TEST(test_get_bin_path) {

    char binPath[MAX_PATH + 1];

    int status = get_bin_path(binPath, MAX_PATH);

    ck_assert_int_eq(status, 1);
    ck_assert_str_ne(binPath, "");

}
END_TEST

START_TEST(test_traverse_up_path) {

    char buffer[MAX_PATH + 1] = {'\0'};  
    char path[] = "/home/pmatkov/programming/c/buzz/libs/tests/bin/test_logger";

    int status = traverse_up_path(buffer, MAX_PATH, path, 2);

    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "/home/pmatkov/programming/c/buzz/libs/tests");

}
END_TEST

START_TEST(test_create_path) {

    char buffer[MAX_PATH + 1] = {'\0'};  

    int status = create_path(buffer, MAX_PATH, "/log");

    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "/home/pmatkov/programming/c/buzz/libs/log");

    status = create_path(buffer, MAX_PATH, "/data/config.conf");

    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "/home/pmatkov/programming/c/buzz/libs/data/config.conf");

}
END_TEST

Suite* path_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Path");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_is_dir);
    tcase_add_test(tc_core, test_get_bin_path);
    tcase_add_test(tc_core, test_traverse_up_path);
    tcase_add_test(tc_core, test_create_path);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = path_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
