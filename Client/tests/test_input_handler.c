#include "../src/input_handler.h"
#include "../src/display.h"

#include <check.h>


START_TEST(test_move_cursor_left) {

    LineEditor *lnEditor = create_line_editor(NULL);

    lnEditor->cursor = PROMPT_SIZE + 1;
    move_cursor_left(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    lnEditor->cursor = PROMPT_SIZE;
    move_cursor_left(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    delete_line_editor(lnEditor);
}
END_TEST

START_TEST(test_move_cursor_right) {

    LineEditor *lnEditor = create_line_editor(NULL);

    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 1;

    move_cursor_right(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + 1);

    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 0;
    move_cursor_right(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    lnEditor->cursor = PROMPT_SIZE + 1;
    lnEditor->charCount = 0;
    move_cursor_right(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + 1);

    delete_line_editor(lnEditor);

}
END_TEST


Suite* inputhandler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Input handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_move_cursor_left);
    tcase_add_test(tc_core, test_move_cursor_right);
    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = inputhandler_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
