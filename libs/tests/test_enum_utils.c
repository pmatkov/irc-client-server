#include "../src/priv_command.h"
#include "../../libs/src/enum_utils.h"
#include "../../libs/src/response_code.h"

#include <check.h>

enum Colors {RED, GREEN, BLUE, UNKNOWN_COLOR, COLORS_COUNT} Colors;
static const char *COLORS_STRINGS[] = {"red", "green", "blue", "unknown"};


START_TEST(test_is_valid_enum_type) {

    ck_assert_int_eq(is_valid_enum_type(GREEN, COLORS_COUNT), 1);
    ck_assert_int_eq(is_valid_enum_type(5, COLORS_COUNT), 0);

}
END_TEST

START_TEST(test_enum_type_to_string) {

    ck_assert_str_eq(enum_type_to_string(BLUE, COLORS_COUNT, COLORS_STRINGS), "blue");
    ck_assert_ptr_eq(enum_type_to_string(5, COLORS_COUNT, COLORS_STRINGS), NULL);
}
END_TEST

START_TEST(test_string_to_enum_type) {

    ck_assert_int_eq(string_to_enum_type(COLORS_STRINGS, COLORS_COUNT, "blue"), BLUE);
    ck_assert_int_eq(string_to_enum_type(COLORS_STRINGS, COLORS_COUNT, "pink"), UNKNOWN_COLOR);

}
END_TEST

Suite* enum_utils_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Enum utils");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_is_valid_enum_type);
    tcase_add_test(tc_core, test_enum_type_to_string);
    tcase_add_test(tc_core, test_string_to_enum_type);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = enum_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
