#ifndef TEST
#define TEST
#endif

#include "../src/priv_input_controller.h"
#include "../src/priv_scrollback_window.h"
#include "../src/priv_input_window.h"
#include "../../libs/src/common.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>

#define ROWS 5
#define COlS 15

static SCREEN *screen = NULL;

static void initialize_test_suite(void) {

    screen = create_terminal();
}

static void cleanup_test_suite(void) {

    endwin();
    delete_terminal(screen);
}

static void add_lines_to_sb(Scrollback *sb, int count) {

    cchar_t buffer[MAX_CHARS + 1] = {'\0'};
    char num[MAX_CHARS + 1] = {'\0'};

    for (int i = 0; i < count && i < sb->capacity; i++) {

        char string[MAX_CHARS + 1] = "message";

        uint_to_str(num, ARRAY_SIZE(num), i + 1);
        strcat(string, num);

        str_to_complex_str(buffer, ARRAY_SIZE(buffer), string, 0);
        add_to_scrollback(sb, buffer);
    }
}

START_TEST(test_get_keyboard_cmd_function) {

    KeyboardCmdFunc keyboardCmdFunc = scroll_line_up;
    ck_assert_ptr_eq(get_keyboard_cmd_function(KEY_CTRLUP), keyboardCmdFunc);

}
END_TEST

START_TEST(test_code_to_window_type) {

    ck_assert_int_eq(code_to_window_type(KEY_CTRLUP), SCROLLBACK_WINDOW);

}
END_TEST

START_TEST(test_scroll_line_up) {

    ScrollbackWindow *scrolbackWindow = create_scrollback_window(ROWS, COLS, 0, 0, ROWS * 2);
    Scrollback *sb = scrolbackWindow->scrollback;

    add_lines_to_sb(sb, ROWS + 1);

    ck_assert_int_eq(sb->count, ROWS + 1);
    ck_assert_int_eq(sb->head, ROWS + 1);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->topLine, 1);
    ck_assert_int_eq(sb->bottomLine, ROWS + 1);

    move_sb_up(sb, 1);

    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, ROWS);

    move_sb_up(sb, 1);

    ck_assert_int_eq(sb->topLine, 0);

    delete_scrollback_window(scrolbackWindow);
}
END_TEST

START_TEST(test_scroll_line_down) {

    ScrollbackWindow *scrolbackWindow = create_scrollback_window(ROWS, COLS, 0, 0, ROWS * 2);
    Scrollback *sb = scrolbackWindow->scrollback;

    add_lines_to_sb(sb, ROWS + 1);

    move_sb_up(sb, 1);
    move_sb_down(sb, 1);

    ck_assert_int_eq(sb->count, ROWS + 1);
    ck_assert_int_eq(sb->head, ROWS + 1);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->topLine, 1);
    ck_assert_int_eq(sb->bottomLine, ROWS + 1);

    move_sb_down(sb, 1);

    ck_assert_int_eq(sb->bottomLine, ROWS + 1);

    delete_scrollback_window(scrolbackWindow);
}
END_TEST

START_TEST(test_scroll_page_up) {

    ScrollbackWindow *scrolbackWindow = create_scrollback_window(ROWS, COLS, 0, 0, ROWS * 3);
    Scrollback *sb = scrolbackWindow->scrollback;

    add_lines_to_sb(sb, ROWS * 2);

    ck_assert_int_eq(sb->count, ROWS * 2);
    ck_assert_int_eq(sb->head, ROWS * 2);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->topLine, ROWS);
    ck_assert_int_eq(sb->bottomLine, ROWS * 2);

    move_sb_up(sb, ROWS);

    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, ROWS);

    delete_scrollback_window(scrolbackWindow);
}
END_TEST

START_TEST(test_scroll_page_down) {

    ScrollbackWindow *scrolbackWindow = create_scrollback_window(ROWS, COLS, 0, 0, ROWS * 3);
    Scrollback *sb = scrolbackWindow->scrollback;

    add_lines_to_sb(sb, ROWS * 2 + 1);

    move_sb_up(sb, ROWS);

    ck_assert_int_eq(sb->topLine, 1);
    ck_assert_int_eq(sb->bottomLine, ROWS + 1);

    move_sb_up(sb, ROWS);

    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, ROWS);

    move_sb_down(sb, ROWS);

    ck_assert_int_eq(sb->topLine, ROWS);
    ck_assert_int_eq(sb->bottomLine, ROWS * 2);

    move_sb_down(sb, ROWS);

    ck_assert_int_eq(sb->topLine, ROWS + 1);
    ck_assert_int_eq(sb->bottomLine, ROWS * 2 + 1);

    delete_scrollback_window(scrolbackWindow);
}
END_TEST

Suite* input_controller_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Input controller");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_get_keyboard_cmd_function);
    tcase_add_test(tc_core, test_code_to_window_type);
    tcase_add_test(tc_core, test_scroll_line_up);
    tcase_add_test(tc_core, test_scroll_line_down);
    tcase_add_test(tc_core, test_scroll_page_up);
    tcase_add_test(tc_core, test_scroll_page_down);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = input_controller_suite();
    sr = srunner_create(s);

    initialize_test_suite();
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif


