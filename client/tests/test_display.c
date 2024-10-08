#include "../src/priv_display.h"
#include "../src/priv_scrollback.h"

#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/mock.h"

#include <check.h>

#define ck_assert_wstr_eq(s1, s2) \
    ck_assert_msg((wcscmp((s1), (s2)) == 0), \
                  "Failed: '%ls' != '%ls'", \
                  s1, s2)

START_TEST(test_create_windows) {

    SCREEN *screen = create_terminal();
    set_mock_stdscr(stdscr);

    WindowManager *windowManager = create_windows();

    ck_assert_ptr_ne(windowManager, NULL);

    delete_windows(windowManager);

    endwin();
    delete_terminal(screen);

}
END_TEST

START_TEST(test_create_print_tokens) {

    PrintTokens *printTokens = create_print_tokens(1, " ** ", NULL, "message");

    ck_assert_ptr_ne(printTokens, NULL);
    ck_assert_int_eq(printTokens->useTimestamp, 1);
    ck_assert_str_eq(printTokens->separator, " ** ");
    ck_assert_ptr_eq(printTokens->origin, NULL);
    ck_assert_str_eq(printTokens->message, "message");

    delete_print_tokens(printTokens);

}
END_TEST

START_TEST(test_printmsg) {

    Scrollback *sb = create_scrollback(NULL, 100);

    printmsg(sb, &(PrintTokens){0, " ** ", NULL, "Test"}, COLOR_SEP(CYAN));

    ck_assert_int_eq(sb->head, 1);
    ck_assert_int_eq(sb->bottomLine, 1);
    ck_assert_int_eq(sb->count, 1);

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (sb->buffer[0][i].chars[0]) {
        wcscat(wstring, sb->buffer[0][i++].chars);
    }

    ck_assert_wstr_eq(wstring, L" ** Test");

    delete_scrollback(sb);

}
END_TEST

START_TEST(test_get_message_length) {

    PrintTokens msgParams = {1, " ** ", NULL, "Test"};

    ck_assert_int_eq(get_message_length(&msgParams), 13);

}
END_TEST

START_TEST(test_string_to_complex_string) {

    cchar_t complexString[MAX_CHARS + 1] = {0};
    int charsConverted = 0;
    
    charsConverted = string_to_complex_string("Buzz v0.1\n x", complexString, strlen("Buzz v0.1\n x"), COLOR_SEP(MAGENTA));

    wchar_t wideString[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (i < charsConverted && complexString[i].chars[0] != L'\0') {
        wcscat(wideString, complexString[i++].chars);
    }
    ck_assert_int_eq(complexString->ext_color, MAGENTA);
    ck_assert_wstr_eq(wideString, L"Buzz v0.1\n x");
}
END_TEST

START_TEST(test_count_complex_chars) {

    cchar_t complexString[MAX_CHARS + 1] = {0};
    int charsConverted = 0;
    
    charsConverted = string_to_complex_string("Buzz v0.1\n x", complexString, strlen("Buzz v0.1\n x"), COLOR_SEP(MAGENTA));

    wchar_t wideString[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (i < charsConverted && complexString[i].chars[0] != L'\0') {
        wcscat(wideString, complexString[i++].chars);
    }
    int count = count_complex_chars(complexString);

    ck_assert_int_eq(count, strlen("Buzz v0.1\n x"));

}
END_TEST

Suite* display_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Display");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_windows);
    tcase_add_test(tc_core, test_create_print_tokens);
    tcase_add_test(tc_core, test_printmsg);
    tcase_add_test(tc_core, test_get_message_length);
    tcase_add_test(tc_core, test_string_to_complex_string);
    tcase_add_test(tc_core, test_count_complex_chars);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = display_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
