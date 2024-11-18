#include "../src/priv_line_editor.h"
#include "../../libs/src/mock.h"

#include <check.h>

#define PROMPT_SIZE 2

void set_content(LineEditor *lnEditor, int cursor, int charCount, const char *content) {

    RegMessage *message = create_reg_message(content);
    enqueue(lnEditor->cmdQueue, message);

    lnEditor->cursor = cursor;
    lnEditor->charCount = charCount;
    lnEditor->cmdQueue->currentItem--;

    delete_message(message);
}

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

START_TEST(test_delete_char) {

    LineEditor *lnEditor = create_line_editor(NULL);

    char *content = "message";

    set_content(lnEditor, PROMPT_SIZE + strlen(content) - 1, strlen(content), content);

    delete_char(lnEditor);
    ck_assert_int_eq(lnEditor->charCount, strlen(content) - 1);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(content) - 1);

    RegMessage *message = get_current_item(lnEditor->cmdQueue);
    ck_assert_str_eq(get_reg_message_content(message), "messag");

    delete_line_editor(lnEditor);

}
END_TEST

START_TEST(test_add_char) {

    LineEditor *lnEditor = create_line_editor(NULL);

    char *content = "message";

    set_content(lnEditor, PROMPT_SIZE + strlen(content), strlen(content), content);

    add_char(lnEditor, '1');
    ck_assert_int_eq(lnEditor->charCount, strlen(content) + 1);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(content) + 1);

    RegMessage *message = get_current_item(lnEditor->cmdQueue);
    ck_assert_str_eq(get_reg_message_content(message), "message1");

    delete_line_editor(lnEditor);

}
END_TEST

START_TEST(test_use_backspace) {

    LineEditor *lnEditor = create_line_editor(NULL);

    char *content = "message";

    set_content(lnEditor, PROMPT_SIZE + strlen(content), strlen(content), content);

    use_backspace(lnEditor);
    ck_assert_int_eq(lnEditor->charCount, strlen(content) - 1);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(content) - 1);

    RegMessage *message = get_current_item(lnEditor->cmdQueue);
    ck_assert_str_eq(get_reg_message_content(message), "messag");

    delete_line_editor(lnEditor);

}
END_TEST

START_TEST(test_use_delete) {

    LineEditor *lnEditor = create_line_editor(NULL);

    char *content = "message";

    set_content(lnEditor, PROMPT_SIZE + strlen(content), strlen(content), content);

    use_delete(lnEditor);
    ck_assert_int_eq(lnEditor->charCount, strlen(content));
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(content));

    RegMessage *message = get_current_item(lnEditor->cmdQueue);
    ck_assert_str_eq(get_reg_message_content(message), "message");

    delete_line_editor(lnEditor);

}
END_TEST

START_TEST(test_get_lneditor_index_and_function) {

    int index1 = get_le_func_index(KEY_LEFT);
    int index2 = get_le_func_index(KEY_RIGHT);

    ck_assert_ptr_eq(get_lneditor_function(index1), move_cursor_left);
    ck_assert_ptr_ne(get_lneditor_function(index2), move_cursor_left);

}
END_TEST

Suite* line_editor_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Line editor");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_line_editor);
    tcase_add_test(tc_core, test_move_cursor_left);
    tcase_add_test(tc_core, test_move_cursor_right);
    tcase_add_test(tc_core, test_delete_char);
    tcase_add_test(tc_core, test_add_char);
    tcase_add_test(tc_core, test_use_backspace);
    tcase_add_test(tc_core, test_use_delete);
    tcase_add_test(tc_core, test_get_lneditor_index_and_function);
    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = line_editor_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
