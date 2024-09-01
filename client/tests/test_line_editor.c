#include "../src/test_line_editor.h"

#include <check.h>

#define PROMPT_SIZE 2

START_TEST(test_create_line_editor) {

    LineEditor *lnEditor = create_line_editor(NULL);

    ck_assert_ptr_ne(lnEditor, NULL);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    ck_assert_int_eq(lnEditor->charCount, 0);

    delete_line_editor(lnEditor);
}
END_TEST

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


Suite* lineeditor_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Line editor");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_line_editor);
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

    s = lineeditor_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
