#ifndef TEST
#define TEST
#endif

#include "../src/priv_base_window.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <ncursesw/curses.h>

#define ROWS 5
#define COlS 10

static SCREEN *screen = NULL;

static void initialize_test_suite(void) {

    screen = create_terminal();
}

static void cleanup_test_suite(void) {

    endwin();
    delete_terminal(screen);
}

START_TEST(test_create_base_window) {

    BaseWindow *baseWindow = create_base_window(ROWS, COLS, 0, 0, BASE_WINDOW);

    ck_assert_ptr_ne(baseWindow, NULL);
    ck_assert_ptr_ne(baseWindow->window, NULL);
    ck_assert_int_eq(baseWindow->rows, ROWS);
    ck_assert_int_eq(baseWindow->cols, COLS);
    ck_assert_int_eq(baseWindow->windowType, BASE_WINDOW);

    delete_base_window(baseWindow);

}
END_TEST

START_TEST(test_create_window_group) {

    WindowGroup *windowGroup = create_window_group(0);

    ck_assert_ptr_ne(windowGroup, NULL);
    ck_assert_ptr_ne(windowGroup->baseWindows, NULL);
    ck_assert_int_eq(windowGroup->activeWindowIndex, 0);
    ck_assert_int_eq(windowGroup->count, 0);
    ck_assert_int_eq(windowGroup->capacity, DEF_WINDOW_COUNT);

    delete_window_group(windowGroup);

}
END_TEST

START_TEST(test_add_remove_window) {

    WindowGroup *windowGroup = create_window_group(0);

    BaseWindow *baseWindow = create_base_window(ROWS, COLS, 0, 0, BASE_WINDOW);

    add_window(windowGroup, baseWindow);
    ck_assert_int_eq(windowGroup->count, 1);

    remove_window(windowGroup);
    ck_assert_int_eq(windowGroup->count, 0);

    delete_window_group(windowGroup);
}
END_TEST

START_TEST(test_get_set_active_window) {

    WindowGroup *windowGroup = create_window_group(0);

    ck_assert_ptr_ne(windowGroup, NULL);
    ck_assert_ptr_ne(windowGroup->baseWindows, NULL);
    ck_assert_int_eq(windowGroup->count, 0);
    ck_assert_int_eq(windowGroup->capacity, DEF_WINDOW_COUNT);
    ck_assert_int_eq(windowGroup->activeWindowIndex, 0);

    set_active_window(windowGroup, 1);
    ck_assert_int_eq(windowGroup->activeWindowIndex, 1);

    delete_window_group(windowGroup);
}
END_TEST

START_TEST(test_get_set_window_items) {

    BaseWindow *baseWindow = create_base_window(ROWS, COLS, 0, 0, BASE_WINDOW);

    ck_assert_ptr_ne(baseWindow, NULL);
    ck_assert_ptr_ne(get_window(baseWindow), NULL);
    ck_assert_int_eq(get_window_type(baseWindow), BASE_WINDOW);
    ck_assert_int_eq(get_window_rows(baseWindow), ROWS);
    ck_assert_int_eq(get_window_cols(baseWindow), COLS);

    delete_base_window(baseWindow);

}
END_TEST

Suite* base_window_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Base window");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_base_window);
    tcase_add_test(tc_core, test_create_window_group);
    tcase_add_test(tc_core, test_add_remove_window);
    tcase_add_test(tc_core, test_get_set_active_window);
    tcase_add_test(tc_core, test_get_set_window_items);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = base_window_suite();
    sr = srunner_create(s);

    initialize_test_suite();
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif
