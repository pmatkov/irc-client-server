#ifndef TEST
#define TEST
#endif

#include "../src/priv_scrollback_window.h"
#include "../../libs/src/common.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>

#define ROWS 5
#define COlS 15

#define ck_assert_wstr_eq(s1, s2) \
    ck_assert_msg((wcscmp((s1), (s2)) == 0), \
                  "Failed: %ls != %ls", \
                  s1, s2)


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

START_TEST(test_create_scrollback_window) {

    ScrollbackWindow *scrolbackWindow = create_scrollback_window(ROWS, COLS, 0, 0, ROWS);

    ck_assert_ptr_ne(scrolbackWindow, NULL);
    ck_assert_ptr_ne(scrolbackWindow->scrollback, NULL);
    ck_assert_int_eq(scrolbackWindow->baseWindow.rows, ROWS);
    ck_assert_int_eq(scrolbackWindow->baseWindow.cols, COLS);

    delete_scrollback_window(scrolbackWindow);

}
END_TEST

START_TEST(test_create_scrollback) {

    Scrollback *sb = create_scrollback(ROWS, 2);

    ck_assert_ptr_ne(sb, NULL);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->head, 0);
    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, 0);
    ck_assert_int_eq(sb->capacity, ROWS * 2);
    ck_assert_int_eq(sb->count, 0);

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_is_scrollback_empty) {

    Scrollback *sb = create_scrollback(ROWS, 2);

    ck_assert_int_eq(is_scrollback_empty(sb), 1);

    delete_scrollback(sb); 
}
END_TEST

START_TEST(test_is_scrollback_full) {

    Scrollback *sb = create_scrollback(ROWS, 2);

    ck_assert_int_eq(is_scrollback_full(sb), 0); 

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_add_to_scrollback) {

    Scrollback *sb = create_scrollback(ROWS, 2);

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = buffer;
    
    bufferPtr += str_to_complex_str(bufferPtr, ARRAY_SIZE(buffer), "12:00", 0);
    bufferPtr += str_to_complex_str(bufferPtr, ARRAY_SIZE(buffer), " ## ", COLOR_SEP(MAGENTA));
    bufferPtr += str_to_complex_str(bufferPtr, ARRAY_SIZE(buffer), "Buzz v0.1", 0);

    add_to_scrollback(sb, buffer);

    ck_assert_int_eq(sb->count, 1);
    ck_assert_int_eq(sb->head, 1);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, 1);

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (sb->buffer[0][i].chars[0] != L'\0') {
        wcscat(wstring, sb->buffer[0][i++].chars);
    }
    ck_assert_wstr_eq(wstring, L"12:00 ## Buzz v0.1");

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_move_sb_up_down) {

    Scrollback *sb = create_scrollback(ROWS, 2);

    add_lines_to_sb(sb, ROWS + 1);

    ck_assert_int_eq(sb->count, ROWS + 1);
    ck_assert_int_eq(sb->head, ROWS + 1);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->topLine, 1);
    ck_assert_int_eq(sb->bottomLine, ROWS + 1);

    move_sb_up(sb, 1);
    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, ROWS);

    move_sb_down(sb, 1);
    ck_assert_int_eq(sb->topLine, 1);
    ck_assert_int_eq(sb->bottomLine, ROWS + 1);

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_count_remaining_top_lines) {

    Scrollback *sb = create_scrollback(ROWS, 3);

    add_lines_to_sb(sb, ROWS * 2 + 1);

    int count = count_remaining_top_lines(sb);

    ck_assert_int_eq(count, ROWS + 1);

    move_sb_up(sb, ROWS);

    count = count_remaining_top_lines(sb);

    ck_assert_int_eq(count, 1);

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_count_remaining_bottom_lines) {

    Scrollback *sb = create_scrollback(ROWS, 3);

    add_lines_to_sb(sb, ROWS * 2 + 1);

    int count = count_remaining_bottom_lines(sb);

    ck_assert_int_eq(count, 0);

    move_sb_up(sb, ROWS);

    count = count_remaining_bottom_lines(sb);

    ck_assert_int_eq(count, ROWS);

    move_sb_up(sb, ROWS);

    count = count_remaining_bottom_lines(sb);

    ck_assert_int_eq(count, ROWS + 1);

    delete_scrollback(sb);
}
END_TEST

Suite* scrollback_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Scrollback window");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_scrollback_window);
    tcase_add_test(tc_core, test_create_scrollback);
    tcase_add_test(tc_core, test_is_scrollback_empty);
    tcase_add_test(tc_core, test_is_scrollback_full);
    tcase_add_test(tc_core, test_add_to_scrollback);
    tcase_add_test(tc_core, test_move_sb_up_down);
    tcase_add_test(tc_core, test_count_remaining_top_lines);
    tcase_add_test(tc_core, test_count_remaining_bottom_lines);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = scrollback_suite();
    sr = srunner_create(s);

    initialize_test_suite();
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif


