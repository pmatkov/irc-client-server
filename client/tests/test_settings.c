#include "../src/priv_settings.h"

#include <check.h>

START_TEST(test_create_settings) {

    Settings *settings = create_settings();

    ck_assert_ptr_ne(settings, NULL);

    delete_settings(settings);
}
END_TEST

START_TEST(test_string_to_property_type) {

    PropertyType propertyType = string_to_property_type("test");

    ck_assert_int_eq(propertyType, UNKNOWN_PROPERTY_TYPE);

    propertyType = string_to_property_type("nickname");

    ck_assert_int_eq(propertyType, NICKNAME);
}
END_TEST

START_TEST(test_is_valid_property_type) {

    int valid = is_valid_property_type(10);

    ck_assert_int_eq(valid, 0);

    valid = is_valid_property_type(NICKNAME);

    ck_assert_int_eq(valid, 1);

}
END_TEST


START_TEST(test_set_default_settings) {

    Settings *settings = create_settings();
    set_default_settings(settings);

    ck_assert_str_eq(get_property_value(settings, USERNAME), "pmatkov");
    ck_assert_str_eq(get_property_value(settings, COLOR), "1");

    delete_settings(settings);

}
END_TEST

START_TEST(test_get_property_type_string) {

    ck_assert_str_eq(property_type_to_string(NICKNAME), "nickname");

}
END_TEST

START_TEST(test_get_property_value_assigned) {

    Settings *settings = create_settings();
    set_property_value(settings, NICKNAME, "john");

    ck_assert_str_eq(get_property_value(settings, NICKNAME), "john");
    ck_assert_int_eq(is_property_assigned(settings, NICKNAME), 1);

    ck_assert_str_eq(get_property_value(settings, USERNAME), "");
    ck_assert_int_eq(is_property_assigned(settings, USERNAME), 0);

    delete_settings(settings);
}
END_TEST

START_TEST(test_get_assigned_properties) {

    Settings *settings = create_settings();
    set_default_settings(settings);

    ck_assert_int_eq(get_assigned_properties(settings), 4);

    delete_settings(settings);
}
END_TEST

START_TEST(test_write_settings) {

    Settings *settings = create_settings();

    set_property_value(settings, NICKNAME, "john");

    write_settings(settings, "tests/data/settings.conf");

    delete_settings(settings);

}
END_TEST

START_TEST(test_read_settings) {

    Settings *settings = create_settings();

    read_settings(settings, "tests/data/settings.conf");

    ck_assert_str_eq(get_property_value(settings, NICKNAME), "john");

    ck_assert_int_eq(get_assigned_properties(settings), 1);

    delete_settings(settings);
}
END_TEST

Suite* settings_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Settings");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_settings);
    tcase_add_test(tc_core, test_string_to_property_type);
    tcase_add_test(tc_core, test_is_valid_property_type);
    tcase_add_test(tc_core, test_set_default_settings);
    tcase_add_test(tc_core, test_get_property_type_string);
    tcase_add_test(tc_core, test_get_property_value_assigned);
    tcase_add_test(tc_core, test_get_assigned_properties);
    tcase_add_test(tc_core, test_write_settings);
    tcase_add_test(tc_core, test_read_settings);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = settings_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
