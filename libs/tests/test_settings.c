#include "../src/priv_settings.h"
#include "../src/string_utils.h"

#include <check.h>

enum OptionType {OT_NICKNAME, OT_USERNAME, OT_PORT, OPTION_TYPE_COUNT};

static Settings *settings = NULL;

static void initialize_test_suite(void) {

    settings = create_settings(OPTION_TYPE_COUNT);
}

static void cleanup_test_suite(void) {

    delete_settings(settings);
}

static void reset_settings(void) {

    settings->count = 0;

    for (int i = 0; i < settings->capacity; i++) {

       reset_option(i);
    }
}

START_TEST(test_create_settings) {

    ck_assert_ptr_ne(settings, NULL);
    ck_assert_int_eq(settings->capacity, OPTION_TYPE_COUNT);
    ck_assert_int_eq(settings->count, 0);

}
END_TEST

START_TEST(test_register_option) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "john");

    ck_assert_int_eq(settings->count, 1);
    ck_assert_int_eq(is_option_registered(OT_NICKNAME), 1);

    reset_settings();
}
END_TEST

START_TEST(test_unregister_option) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "john");

    ck_assert_int_eq(settings->count, 1);
    ck_assert_int_eq(is_option_registered(OT_NICKNAME), 1);

    unregister_option(OT_NICKNAME);

    ck_assert_int_eq(settings->count, 0);
    ck_assert_int_eq(is_option_registered(OT_NICKNAME), 0);

    reset_settings();
}
END_TEST

START_TEST(test_get_set_option_data) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "john");
    ck_assert_str_eq(get_char_option_value(OT_NICKNAME), "john");

    ck_assert_int_eq(get_option_data_type(OT_NICKNAME), CHAR_TYPE);
    ck_assert_str_eq(get_option_label(OT_NICKNAME), "nickname");

    set_option_value(OT_NICKNAME, "mark");
    ck_assert_str_eq(get_char_option_value(OT_NICKNAME), "mark");

    register_option(INT_TYPE, OT_PORT, "port", &((int){10}));
    ck_assert_int_eq(get_int_option_value(OT_PORT), 10);

    reset_settings();
}
END_TEST

START_TEST(test_is_option_registered) {

    ck_assert_int_eq(is_option_registered(OT_NICKNAME), 0);

}
END_TEST

START_TEST(test_is_valid_option_type) {

    ck_assert_int_eq(is_valid_option_type(OT_USERNAME), 1);
    ck_assert_int_eq(is_valid_option_type(10), 0);

}
END_TEST

START_TEST(test_read_settings) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "");
    
    read_settings("tests/data/settings.conf");
    ck_assert_str_eq(get_char_option_value(OT_NICKNAME), "steve");

    reset_settings();

}
END_TEST

START_TEST(test_write_settings) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "steve");

    write_settings("tests/data/settings.conf");

    reset_settings();

}
END_TEST

START_TEST(test_label_to_option_type) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "");

    ck_assert_int_eq(label_to_option_type("nickname"), OT_NICKNAME);
    ck_assert_int_eq(label_to_option_type("username"), -1);

    reset_settings();

}
END_TEST

START_TEST(test_read_option_string) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "");

    char buffer[] = "nickname=john";
    read_option_string(buffer);
    ck_assert_str_eq(get_char_option_value(OT_NICKNAME), "john");

    reset_settings();

}
END_TEST

START_TEST(test_create_option_string) {

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", "john");

    char buffer[MAX_CHARS + 1];
    create_option_string(buffer, MAX_CHARS + 1, &settings->options[OT_NICKNAME]);
    ck_assert_str_eq(buffer, "nickname=john");

    reset_settings();

}
END_TEST

Suite* settings_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Settings");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite

    tcase_add_test(tc_core, test_create_settings);
    tcase_add_test(tc_core, test_register_option);
    tcase_add_test(tc_core, test_unregister_option);
    tcase_add_test(tc_core, test_get_set_option_data);
    tcase_add_test(tc_core, test_is_option_registered);
    tcase_add_test(tc_core, test_is_valid_option_type);
    tcase_add_test(tc_core, test_read_settings);
    tcase_add_test(tc_core, test_write_settings);
    tcase_add_test(tc_core, test_label_to_option_type);
    tcase_add_test(tc_core, test_read_option_string);
    tcase_add_test(tc_core, test_create_option_string);

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

    initialize_test_suite();

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif
