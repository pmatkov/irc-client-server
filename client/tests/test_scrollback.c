#include "../src/priv_scrollback.h"
#include "../src/priv_display.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>

#define ck_assert_wstr_eq(s1, s2) \
    ck_assert_msg((wcscmp((s1), (s2)) == 0), \
                  "Failed: %ls != %ls", \
                  s1, s2)

START_TEST(test_create_scrollback) {

    Scrollback *sb = create_scrollback(NULL, 100);

    ck_assert_ptr_ne(sb, NULL);
    ck_assert_ptr_eq(sb->window, NULL);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->head, 0);
    ck_assert_int_eq(sb->topLine, 0);
    ck_assert_int_eq(sb->bottomLine, 0);
    ck_assert_int_eq(sb->capacity, 100);
    ck_assert_int_eq(sb->count, 0);

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_is_scrollback_empty) {

    Scrollback *sb = create_scrollback(NULL, 100);

    ck_assert(is_scrollback_empty(sb));

    delete_scrollback(sb); 
}
END_TEST

START_TEST(test_is_scrollback_full) {

    Scrollback *sb = create_scrollback(NULL, 100);
    sb->count = 100;

    ck_assert(is_scrollback_full(sb)); 

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_add_to_scrollback) {

    Scrollback *sb = create_scrollback(NULL, 5);

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = buffer;
    
    bufferPtr += string_to_complex_string(bufferPtr, ARR_SIZE(buffer), "12:00", 0);
    bufferPtr += string_to_complex_string(bufferPtr, ARR_SIZE(buffer), " ## ", COLOR_SEP(MAGENTA));
    bufferPtr += string_to_complex_string(bufferPtr, ARR_SIZE(buffer), "Buzz v0.1", 0);

    add_to_scrollback(sb, buffer, bufferPtr - buffer);

    ck_assert_int_eq(sb->head, 1);
    ck_assert_int_eq(sb->count, 1);

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (sb->buffer[0][i].chars[0] != L'\0') {
        wcscat(wstring, sb->buffer[0][i++].chars);
    }

    ck_assert_wstr_eq(wstring, L"12:00 ## Buzz v0.1");

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_print_from_scrollback) {

    SCREEN *screen = create_terminal();
    set_mock_stdscr(stdscr);

    Scrollback *sb = create_scrollback(stdscr, 5);

    ck_assert_ptr_ne(sb, NULL);
    ck_assert_ptr_ne(sb->window, NULL);

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = buffer;
    
    bufferPtr += string_to_complex_string(bufferPtr, ARR_SIZE(buffer), "12:00", 0);
    bufferPtr += string_to_complex_string(bufferPtr, ARR_SIZE(buffer), " ## ", COLOR_SEP(MAGENTA));
    bufferPtr += string_to_complex_string(bufferPtr, ARR_SIZE(buffer), "Buzz v0.1", 0);

    add_to_scrollback(sb, buffer, bufferPtr - buffer);
    print_from_scrollback(sb, 5, 1);

    char string[MAX_CHARS + 1] = {'\0'};
    mvwinnstr(stdscr, 5, 0, string, bufferPtr - buffer);

    ck_assert_str_eq(string, "12:00 ## Buzz v0.1");

    delete_scrollback(sb);

    endwin();
    delete_terminal(screen);
}
END_TEST

START_TEST(test_get_scrollback_index_and_function) {

    int index1 = get_sb_func_index(KEY_PPAGE);
    int index2 = get_sb_func_index(KEY_NPAGE);

    ck_assert_ptr_eq(get_scrollback_function(index1), scroll_page_up);
    ck_assert_ptr_ne(get_scrollback_function(index2), scroll_page_up);

}
END_TEST

Suite* scrollback_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Scrollback");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_scrollback);
    tcase_add_test(tc_core, test_is_scrollback_empty);
    tcase_add_test(tc_core, test_is_scrollback_full);
    tcase_add_test(tc_core, test_add_to_scrollback);
    tcase_add_test(tc_core, test_print_from_scrollback);
    tcase_add_test(tc_core, test_get_scrollback_index_and_function);

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

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
