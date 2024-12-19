#include "../src/priv_display.h"
#include "../../libs/src/mock.h"

#include <check.h>

static SCREEN *screen = NULL;

static void initialize_test_suite(void) {

    screen = create_terminal();
}

static void cleanup_test_suite(void) {

    endwin();
    delete_terminal(screen);
}

START_TEST(test_create_window_manager) {

    WindowManager *windowManager = create_window_manager(0, 0);

    ck_assert_ptr_ne(windowManager, NULL);
    ck_assert_ptr_ne(windowManager->stdscr, NULL);
    ck_assert_ptr_ne(windowManager->mainWindows, NULL);
    ck_assert_ptr_ne(windowManager->titlewin, NULL);
    ck_assert_ptr_ne(windowManager->statuswin, NULL);
    ck_assert_ptr_ne(windowManager->inputwin, NULL);
    ck_assert_ptr_ne(windowManager->observer, NULL);

    delete_window_manager(windowManager);

}
END_TEST


Suite* display_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Display");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_window_manager);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    initialize_test_suite();

    s = display_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif


