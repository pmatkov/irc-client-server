#include "../src/priv_settings.h"
#include "../src/priv_lookup_table.h"
#include "../src/string_utils.h"

#include <check.h>

#define MAX_SETTINGS 5

enum clientProperty {NICKNAME, USERNAME, PORT};
static int keys[] = {NICKNAME, USERNAME, PORT};
static const char *labels[] = {"nickname", "username", "port"};


START_TEST(test_create_settings) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    ck_assert_ptr_ne(settings, NULL);
    ck_assert_int_eq(settings->capacity, MAX_SETTINGS);
    ck_assert_int_eq(settings->count, 0);

    delete_settings(settings);
    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_register_property) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    register_property(settings, CHAR_TYPE, get_lookup_pair(lookupTable, NICKNAME), "john");
    ck_assert_int_eq(is_property_registered(settings, NICKNAME), 1);

    delete_settings(settings);
    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_get_set_property_value) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    register_property(settings, CHAR_TYPE, get_lookup_pair(lookupTable, NICKNAME), "john");

    ck_assert_str_eq((char *)get_property_value(settings, NICKNAME), "john");

    set_property_value(settings, NICKNAME, "mark");
    ck_assert_str_eq((char *) get_property_value(settings, NICKNAME), "mark");

    register_property(settings, INT_TYPE, get_lookup_pair(lookupTable, PORT), &((int){10}));

    ck_assert_int_eq(*((int *) get_property_value(settings, PORT)), 10);

    delete_settings(settings);
    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_is_valid_property) {

    Settings *settings = create_settings(MAX_SETTINGS);

    ck_assert_int_eq(is_valid_property(settings, USERNAME), 1);
    ck_assert_int_eq(is_valid_property(settings, 10), 0);

    delete_settings(settings);
}
END_TEST

START_TEST(test_write_settings) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    register_property(settings, CHAR_TYPE, get_lookup_pair(lookupTable, NICKNAME), "steve");

    write_settings(settings, "tests/data/settings.conf");

    delete_settings(settings);
    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_read_settings) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    register_property(settings, CHAR_TYPE, get_lookup_pair(lookupTable, NICKNAME), "");
    read_settings(settings, lookupTable, "tests/data/settings.conf");

    ck_assert_str_eq(get_property_value(settings, NICKNAME), "steve");

    delete_settings(settings);
    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_read_property_string) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    register_property(settings, CHAR_TYPE, get_lookup_pair(lookupTable, NICKNAME), "");

    char buffer[] = "nickname=john";
    read_property_string(settings, lookupTable, buffer);
    ck_assert_str_eq((char *) get_property_value(settings, NICKNAME), "john");

    delete_settings(settings);
    delete_lookup_table(lookupTable);
}
END_TEST

START_TEST(test_create_property_string) {

    LookupTable *lookupTable = create_lookup_table(keys, labels, ARR_SIZE(keys));
    Settings *settings = create_settings(MAX_SETTINGS);

    register_property(settings, CHAR_TYPE, get_lookup_pair(lookupTable, NICKNAME), "john");

    char buffer[MAX_CHARS + 1];
    create_property_string(buffer, MAX_CHARS + 1, &settings->properties[NICKNAME]);
    ck_assert_str_eq(buffer, "nickname=john");

    delete_settings(settings);
    delete_lookup_table(lookupTable);
}
END_TEST

Suite* settings_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Settings");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite

    tcase_add_test(tc_core, test_create_settings);
    tcase_add_test(tc_core, test_register_property);
    tcase_add_test(tc_core, test_get_set_property_value);
    tcase_add_test(tc_core, test_is_valid_property);
    tcase_add_test(tc_core, test_write_settings);
    tcase_add_test(tc_core, test_read_settings);
    tcase_add_test(tc_core, test_read_property_string);
    tcase_add_test(tc_core, test_create_property_string);

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
