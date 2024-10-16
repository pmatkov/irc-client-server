#ifndef TEST
#define TEST
#endif

#include "../src/priv_display.h"
#include "../src/priv_scrollback.h"

#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/mock.h"

#include <check.h>

#define ck_assert_wstr_eq(s1, s2) \
    ck_assert_msg((wcscmp((s1), (s2)) == 0), \
                  "Failed: '%ls' != '%ls'", \
                  s1, s2)


void test_print_function(WindowManager *windowManager, cchar_t *string, int size) {

    return;
}

START_TEST(test_compress_bits) {

    int result = 0;

    result = COMPRESS_BITS(0, 3, BIT_MASK_SEP);
    ck_assert_int_eq(result, 0xFF);

    result = COMPRESS_BITS(1, 4, BIT_MASK_ORG);
    ck_assert_int_eq(result, 0xFF);

    result = COMPRESS_BITS(2, 5, BIT_MASK_CNT);
    ck_assert_int_eq(result, 0xFF);
}
END_TEST

START_TEST(test_get_remaining_cchars) {

    cchar_t buffer[MAX_CHARS + 1] = {0};
    
    int charsConverted = string_to_complex_string(buffer, ARR_SIZE(buffer), "Buzz v0.1\n", COLOR_SEP(MAGENTA));
    ck_assert_int_eq(charsConverted, strlen("Buzz v0.1\n"));

    int remainingChars = get_remaining_cchars(buffer);
    ck_assert_int_eq(remainingChars, sizeof(buffer)/ sizeof(buffer[0]) - charsConverted);

}
END_TEST

START_TEST(test_create_windows) {

    SCREEN *screen = create_terminal();
    set_mock_stdscr(stdscr);

    WindowManager *windowManager = create_windows(0);

    ck_assert_ptr_ne(windowManager, NULL);
    ck_assert_ptr_ne(windowManager->stdscr, NULL);
    ck_assert_ptr_ne(windowManager->titlewin, NULL);
    ck_assert_ptr_ne(windowManager->chatwin, NULL);
    ck_assert_ptr_ne(windowManager->statuswin, NULL);
    ck_assert_ptr_ne(windowManager->inputwin, NULL);

    delete_windows(windowManager);

    endwin();
    delete_terminal(screen);

}
END_TEST

START_TEST(test_create_print_tokens) {

    PrintTokens *printTokens = create_print_tokens(1, " ** ", NULL, "message", 0);

    ck_assert_ptr_ne(printTokens, NULL);
    ck_assert_int_eq(printTokens->useTimestamp, 1);
    ck_assert_str_eq(printTokens->separator, " ** ");
    ck_assert_ptr_eq(printTokens->origin, NULL);
    ck_assert_str_eq(printTokens->content, "message");
    ck_assert_int_eq(printTokens->format, 0);

    delete_print_tokens(printTokens);

}
END_TEST

START_TEST(test_printstr) {

    SCREEN *screen = create_terminal();
    set_mock_stdscr(stdscr);

    WindowManager *windowManager = create_windows(0);

    Scrollback *sb = windowManager->chatwin->backingStore->scrollback;

    printstr(&(PrintTokens){0, " ** ", NULL, "Test", COLOR_SEP(CYAN)}, windowManager);

    ck_assert_int_eq(sb->head, 1);
    ck_assert_int_eq(sb->bottomLine, 1);
    ck_assert_int_eq(sb->count, 1);

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (sb->buffer[0][i].chars[0]) {
        wcscat(wstring, sb->buffer[0][i++].chars);
    }

    ck_assert_wstr_eq(wstring, L" ** Test");

    delete_windows(windowManager);

    endwin();
    delete_terminal(screen);

}
END_TEST

START_TEST(test_string_to_complex_string) {

    cchar_t buffer[MAX_CHARS + 1] = {0};
    int charsConverted = 0;
    
    charsConverted = string_to_complex_string(buffer, ARR_SIZE(buffer), "Buzz v0.1\n x", COLOR_SEP(MAGENTA));

    wchar_t wideString[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (i < charsConverted && buffer[i].chars[0] != L'\0') {
        wcscat(wideString, buffer[i++].chars);
    }
    ck_assert_int_eq(buffer->ext_color, MAGENTA);
    ck_assert_wstr_eq(wideString, L"Buzz v0.1\n x");
}
END_TEST

START_TEST(test_count_complex_chars) {

    cchar_t buffer[MAX_CHARS + 1] = {0};
    int charsConverted = 0;
    
    charsConverted = string_to_complex_string(buffer, ARR_SIZE(buffer), "Buzz v0.1\n x", COLOR_SEP(MAGENTA));

    wchar_t wideString[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (i < charsConverted && buffer[i].chars[0] != L'\0') {
        wcscat(wideString, buffer[i++].chars);
    }
    int count = count_complex_chars(buffer);

    ck_assert_int_eq(count, strlen("Buzz v0.1\n x"));

}
END_TEST

START_TEST(test_get_set_print_func) {

    PrintFunc pf = test_print_function;

    ck_assert_ptr_eq(get_print_function(), print_complex_string);

    set_print_function(pf);

    ck_assert_ptr_eq(get_print_function(), pf);

}
END_TEST

Suite* display_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Display");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_compress_bits);
    tcase_add_test(tc_core, test_get_remaining_cchars);
    tcase_add_test(tc_core, test_create_windows);
    tcase_add_test(tc_core, test_create_print_tokens);
    tcase_add_test(tc_core, test_printstr);
    tcase_add_test(tc_core, test_string_to_complex_string);
    tcase_add_test(tc_core, test_count_complex_chars);
    tcase_add_test(tc_core, test_get_set_print_func);

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
