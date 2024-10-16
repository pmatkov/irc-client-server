#include "../src/priv_ui_window.h"
#include "../src/priv_scrollback.h"

#include <check.h>

START_TEST(test_create_ui_window) {

    UIWindow *uiWindow = create_ui_window(NULL, NULL, NO_BACKING);

    ck_assert_ptr_ne(uiWindow, NULL);
    ck_assert_ptr_eq(uiWindow->window, NULL);
    ck_assert_ptr_eq(uiWindow->backingStore, NULL);
    ck_assert_int_eq(uiWindow->backingType, NO_BACKING);

    delete_ui_window(uiWindow);

}
END_TEST

START_TEST(test_create_backing_store) {

    BackingStore *backingStore = create_backing_store(create_scrollback(NULL, 0), SCROLLBACK);

    ck_assert_ptr_ne(backingStore, NULL);
    ck_assert_ptr_ne(backingStore->scrollback, NULL);

    delete_backing_store(backingStore, SCROLLBACK);
}
END_TEST

START_TEST(test_get_window_items) {

    UIWindow *uiWindow = create_ui_window(NULL, create_scrollback(NULL, 0), SCROLLBACK);

    ck_assert_ptr_ne(uiWindow, NULL);
    ck_assert_ptr_eq(get_window(uiWindow), NULL);
    ck_assert_ptr_ne(get_scrollback(uiWindow), NULL);
    ck_assert_ptr_eq(get_line_editor(uiWindow), NULL);

    delete_ui_window(uiWindow);

}
END_TEST


Suite* ui_window_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("UI window");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_ui_window);
    tcase_add_test(tc_core, test_create_backing_store);
    tcase_add_test(tc_core, test_get_window_items);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = ui_window_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
