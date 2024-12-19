#ifndef TEST
#define TEST
#endif

#include "../src/priv_input_window.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>

#define ROWS 5
#define COLS 15

static SCREEN *screen = NULL;
static LineEditor *lnEditor = NULL;

static void initialize_test_suite(void) {

    lnEditor = create_line_editor(COLS, 0);
    screen = create_terminal();
}

static void cleanup_test_suite(void) {

    delete_line_editor(lnEditor);
    endwin();
    delete_terminal(screen);
}

static void reset_line_editor(void) {

    lnEditor->charCount = 0;
    lnEditor->cursor = PROMPT_SIZE;
}

void set_le_content(LineEditor *lnEditor, int cursor, int charCount, const char *content) {

    Message *message = get_current_item(lnEditor->frontCmdQueue);
    set_message_content(message, content);

    lnEditor->cursor = cursor;
    lnEditor->charCount = charCount;

}

START_TEST(test_create_input_window) {

    InputWindow *inputWindow = create_input_window(ROWS, COLS, 0, 0, COLS);

    ck_assert_ptr_ne(inputWindow, NULL);
    ck_assert_ptr_ne(inputWindow->lineEditor, NULL);
    ck_assert_int_eq(inputWindow->baseWindow.rows, ROWS);
    ck_assert_int_eq(inputWindow->baseWindow.cols, COLS);

    delete_input_window(inputWindow);

}
END_TEST

START_TEST(test_create_line_editor) {

    ck_assert_ptr_ne(lnEditor, NULL);
    ck_assert_ptr_ne(lnEditor->frontCmdQueue, NULL);
    ck_assert_ptr_ne(lnEditor->backCmdQueue, NULL);
    ck_assert_int_eq(lnEditor->inputWidth, COLS);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);
    ck_assert_int_eq(lnEditor->charCount, 0);

    reset_line_editor();
}
END_TEST

START_TEST(test_move_cursor_left) {

    lnEditor->cursor = PROMPT_SIZE + 1;
    move_le_cursor_left(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    lnEditor->cursor = PROMPT_SIZE;
    move_le_cursor_left(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    reset_line_editor();
}
END_TEST

START_TEST(test_move_cursor_right) {

    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 1;

    move_le_cursor_right(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + 1);

    lnEditor->cursor = PROMPT_SIZE;
    lnEditor->charCount = 0;
    move_le_cursor_right(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    lnEditor->cursor = PROMPT_SIZE + 1;
    lnEditor->charCount = 0;
    move_le_cursor_right(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + 1);

    reset_line_editor();
}
END_TEST


START_TEST(test_add_char) {

    const char *string = "message";

    set_le_content(lnEditor, PROMPT_SIZE + strlen(string), strlen(string), string);

    add_le_char(lnEditor, '1');
    ck_assert_int_eq(lnEditor->charCount, strlen(string) + 1);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(string) + 1);

    Message *message = get_current_item(lnEditor->frontCmdQueue);
    const char *content = get_message_content(message);
    ck_assert_str_eq(content, "message1");

    delete_line_editor(lnEditor);

}
END_TEST

START_TEST(test_delete_char) {

    const char *string = "message";

    set_le_content(lnEditor, PROMPT_SIZE + strlen(string) - 1, strlen(string), string);

    delete_le_char(lnEditor);
    ck_assert_int_eq(lnEditor->charCount, strlen(string) - 1);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(string) - 1);

    Message *message = get_current_item(lnEditor->frontCmdQueue);
    const char *content = get_message_content(message);
    ck_assert_str_eq(content, "messag");

    reset_line_editor();
}
END_TEST

START_TEST(test_use_backspace) {

    const char *string = "message";

    set_le_content(lnEditor, PROMPT_SIZE + strlen(string), strlen(string), string);

    use_le_backspace(lnEditor);
    ck_assert_int_eq(lnEditor->charCount, strlen(string) - 1);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE + strlen(string) - 1);

    Message *message = get_current_item(lnEditor->frontCmdQueue);
    const char *content = get_message_content(message);
    ck_assert_str_eq(content, "messag");

    reset_line_editor();
}
END_TEST

START_TEST(test_use_delete) {

    const char *string = "message";

    set_le_content(lnEditor, PROMPT_SIZE, strlen(string), string);

    use_le_delete(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    reset_line_editor();
}
END_TEST

START_TEST(test_use_home) {

    const char *string = "message";

    set_le_content(lnEditor, PROMPT_SIZE + strlen(string), strlen(string), string);

    use_le_home(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    Message *message = get_current_item(lnEditor->frontCmdQueue);
    const char *content = get_message_content(message);
    ck_assert_str_eq(content, "message");

    reset_line_editor();
}
END_TEST

START_TEST(test_use_end) {

    const char *string = "message";

    set_le_content(lnEditor, PROMPT_SIZE, strlen(string), string);

    use_le_end(lnEditor);
    ck_assert_int_eq(lnEditor->cursor, strlen(string) + PROMPT_SIZE);

    reset_line_editor();
}
END_TEST

START_TEST(test_can_add_char) {

    const char *string = "very long m";
    
    set_le_content(lnEditor, PROMPT_SIZE + strlen(string), strlen(string), string);

    ck_assert_int_eq(can_add_char(lnEditor), 1);
    add_le_char(lnEditor, 'e');
    ck_assert_int_eq(can_add_char(lnEditor), 0);

    reset_line_editor();
}
END_TEST

Suite* input_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Input window");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_input_window);
    tcase_add_test(tc_core, test_create_line_editor);
    tcase_add_test(tc_core, test_move_cursor_left);
    tcase_add_test(tc_core, test_move_cursor_right);
    tcase_add_test(tc_core, test_add_char);
    tcase_add_test(tc_core, test_delete_char);
    tcase_add_test(tc_core, test_use_backspace);
    tcase_add_test(tc_core, test_use_delete);
    tcase_add_test(tc_core, test_use_home);
    tcase_add_test(tc_core, test_use_end);
    tcase_add_test(tc_core, test_can_add_char);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = input_suite();
    sr = srunner_create(s);

    initialize_test_suite();
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif


