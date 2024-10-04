#include "../src/priv_settings.h"

#include <check.h>

START_TEST(test_set_default_properties) {

    set_default_settings();

    ck_assert_str_eq(get_property_value(REALNAME), "anonymous");
    ck_assert_str_eq(get_property_value(HOSTNAME), "irc.example.com");

}
END_TEST

START_TEST(test_get_set_property_value) {

    set_property_value(NICKNAME, "john");

    ck_assert_str_eq(get_property_value(NICKNAME), "john");

    set_property_value(NICKNAME, "mark");

    ck_assert_str_eq(get_property_value(NICKNAME), "mark");
    ck_assert_str_eq(get_property_value(HOSTNAME), "");

}
END_TEST

START_TEST(test_is_property_assigned) {

    set_property_value(NICKNAME, "john");

    ck_assert_int_eq(is_property_assigned(NICKNAME), 1);
    ck_assert_int_eq(is_property_assigned(HOSTNAME), 0);

}
END_TEST

START_TEST(test_get_assigned_properties_count) {

    set_property_value(NICKNAME, "john");
    set_property_value(USERNAME, "jjones");

    ck_assert_int_eq(get_assigned_properties_count(CLIENT_PROPERTY), 2);
    ck_assert_int_eq(get_assigned_properties_count(SERVER_PROPERTY), 0);

}
END_TEST

START_TEST(test_write_settings) {

    set_property_value(NICKNAME, "john");

    write_settings("tests/data/settings.conf", CLIENT_PROPERTY);

}
END_TEST

START_TEST(test_read_settings) {

    read_settings("tests/data/settings.conf", CLIENT_PROPERTY);

    ck_assert_int_eq(get_assigned_properties_count(CLIENT_PROPERTY), 1);
    ck_assert_str_eq(get_property_value(NICKNAME), "john");

}
END_TEST

START_TEST(test_string_to_property_type) {

    PropertyType propertyType = string_to_property_type("test");

    ck_assert_int_eq(propertyType, UNKNOWN_PROPERTY_TYPE);

    propertyType = string_to_property_type("nickname");

    ck_assert_int_eq(propertyType, NICKNAME);

}
END_TEST

START_TEST(test_property_type_to_string) {

    const char *string = property_type_to_string(10);

    ck_assert_str_eq(string, "unknown");

    string = property_type_to_string(NICKNAME);

    ck_assert_str_eq(string, "nickname");

}
END_TEST

START_TEST(test_is_valid_property_type) {

    int valid = is_valid_property_type(10);

    ck_assert_int_eq(valid, 0);

    valid = is_valid_property_type(NICKNAME);

    ck_assert_int_eq(valid, 1);

}
END_TEST

Suite* settings_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Settings");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_set_default_properties);
    tcase_add_test(tc_core, test_get_set_property_value);
    tcase_add_test(tc_core, test_is_property_assigned);
    tcase_add_test(tc_core, test_get_assigned_properties_count);
    tcase_add_test(tc_core, test_write_settings);
    tcase_add_test(tc_core, test_read_settings);
    tcase_add_test(tc_core, test_string_to_property_type);
    tcase_add_test(tc_core, test_property_type_to_string);
    tcase_add_test(tc_core, test_is_valid_property_type);

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
