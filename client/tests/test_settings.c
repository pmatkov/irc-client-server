#include "../src/settings.h"

#include <check.h>

START_TEST(test_create_settings_collection) {

        SettingsCollection *settingsCollection = create_settings_collection();

        ck_assert_ptr_ne(settingsCollection, NULL);
        ck_assert_int_eq(settingsCollection->allocatedSize, SETTING_TYPE_COUNT);

        delete_settings_collection(settingsCollection);
}
END_TEST

START_TEST(test_string_to_setting_type) {

    SettingType settingType = string_to_setting_type("test");

    ck_assert_int_eq(settingType, UNKNOWN_SETTING_TYPE);

    settingType = string_to_setting_type("color");

    ck_assert_int_eq(settingType, COLOR);

}
END_TEST

START_TEST(test_is_valid_setting) {

    int valid = is_valid_setting("test");

    ck_assert_int_eq(valid, 0);

    valid = is_valid_setting("color");

    ck_assert_int_eq(valid, 1);

}
END_TEST


START_TEST(test_get_setting_string) {

    char *setting = get_setting_string(COLOR);

    ck_assert_str_eq(setting, "color");

    setting = get_setting_string(10);

    ck_assert_str_eq(setting, NULL);

}
END_TEST

START_TEST(test_set_setting) {

    SettingsCollection *settingsCollection = create_settings_collection();

    set_setting("color", "blue");

    ck_assert_ptr_ne(&settingsCollection->settings[COLOR], NULL);
    ck_assert_int_eq(settingsCollection->settings[COLOR].settingType, COLOR);
    ck_assert_str_eq(settingsCollection->settings[COLOR].key, "color");
    ck_assert_str_eq(settingsCollection->settings[COLOR].value, "blue");

    delete_settings_collection(settingsCollection);

}
END_TEST

Suite* settings_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Settings");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_settings_collection);
    tcase_add_test(tc_core, test_string_to_setting_type);
    tcase_add_test(tc_core, test_is_valid_setting);
    tcase_add_test(tc_core, test_get_setting_string);
    tcase_add_test(tc_core, test_set_setting);

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
