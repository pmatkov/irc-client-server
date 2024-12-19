// #ifndef TEST
// #define TEST
// #endif

#include "../src/priv_print_manager.h"
#include "../src/priv_scrollback_window.h"
#include "../../libs/src/common.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/string_utils.h"
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

START_TEST(test_create_message_tokens) {

    MessageTokens *messageTokens = create_message_tokens(1, " ** ", NULL, "message", 0);

    ck_assert_ptr_ne(messageTokens, NULL);
    ck_assert_int_eq(messageTokens->useTimestamp, 1);
    ck_assert_str_eq(messageTokens->separator, " ** ");
    ck_assert_ptr_eq(messageTokens->origin, NULL);
    ck_assert_str_eq(messageTokens->content, "message");
    ck_assert_int_eq(messageTokens->format, 0);

    delete_message_tokens(messageTokens);

}
END_TEST

START_TEST(test_concat_msg_tokens) {

    cchar_t buffer[MAX_CHARS + 1] = {0};

    concat_msg_tokens(buffer, ARRAY_SIZE(buffer), &(MessageTokens){0, " ** ", NULL, "Test", COLOR_SEP(CYAN)});

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (buffer[i].chars[0]) {
        wcscat(wstring, buffer[i++].chars);
    }

    ck_assert_wstr_eq(wstring, L" ** Test");

}
END_TEST

START_TEST(test_print_tokens) {

    ScrollbackWindow *scrolbackWindow = create_scrollback_window(ROWS, COLS, 0, 0, ROWS);

    print_tokens(&scrolbackWindow->baseWindow, &(MessageTokens){0, " ** ", NULL, "Test", COLOR_SEP(CYAN)});

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (scrolbackWindow->scrollback->buffer[0][i].chars[0]) {
        wcscat(wstring, scrolbackWindow->scrollback->buffer[0][i++].chars);
    }

    ck_assert_wstr_eq(wstring, L" ** Test");

    delete_scrollback_window(scrolbackWindow);

}
END_TEST


Suite* print_manager_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Print manager");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_message_tokens);
    tcase_add_test(tc_core, test_concat_msg_tokens);
    tcase_add_test(tc_core, test_print_tokens);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = print_manager_suite();
    sr = srunner_create(s);

    initialize_test_suite();
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif


