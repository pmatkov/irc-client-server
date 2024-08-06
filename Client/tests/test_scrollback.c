#include "../src/scrollback.h"
#include "../src/display.h"

#include <check.h>

#define ck_assert_wstr_eq(s1, s2) \
    ck_assert_msg((wcscmp((s1), (s2)) == 0), \
                  "Failed: %ls != %ls", \
                  s1, s2)

START_TEST(test_create_sb) {

    Scrollback *sb = create_scrollback(NULL, 100);

    ck_assert_ptr_ne(sb, NULL);
    ck_assert_ptr_eq(sb->window, NULL);
    ck_assert_int_eq(sb->head, 0);
    ck_assert_int_eq(sb->tail, 0);
    ck_assert_int_eq(sb->currentLine, 0);
    ck_assert_int_eq(sb->allocatedSize, 100);
    ck_assert_int_eq(sb->usedSize, 0);

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_isempty_sb) {

    Scrollback *sb = create_scrollback(NULL, 100);

    ck_assert(is_empty(sb));

    delete_scrollback(sb); 
}
END_TEST

START_TEST(test_isfull_sb) {

    Scrollback *sb = create_scrollback(NULL, 100);
    sb->usedSize = 100;

    ck_assert(is_full(sb)); 

    delete_scrollback(sb);
}
END_TEST

START_TEST(test_get_preceding_ln_count) {

    Scrollback *sb = create_scrollback(NULL, 100);

    sb->tail = 0;
    sb->head = 5;
    sb->currentLine = 5;
    sb->usedSize = 5;

    ck_assert_int_eq(get_preceding_ln_count(sb), 5);

    sb->tail = 1;
    sb->head = 0;
    sb->currentLine = 0;
    sb->usedSize = 100;

    ck_assert_int_eq(get_preceding_ln_count(sb), 100);

    sb->tail = 5;
    sb->head = 4;
    sb->currentLine = 1;
    sb->usedSize = 100;

    ck_assert_int_eq(get_preceding_ln_count(sb), 97);

    delete_scrollback(sb); 
}
END_TEST

START_TEST(test_convert_to_cchar) {

    cchar_t line[100+1] = {0};
    cchar_t *linePtr = line;
    
    linePtr += convert_to_cchar(linePtr, "Buzz v0.1",  COLOR_SEP(MAGENTA));
    linePtr = line;

    wchar_t result[100 + 1] = {L'\0'};

    int i = 0;
    while (linePtr && linePtr[i].chars[0]) {
        wcscat(result, linePtr[i++].chars);
    }
    ck_assert_int_eq(line->ext_color, MAGENTA);
    ck_assert_wstr_eq(L"Buzz v0.1", result);
}
END_TEST

START_TEST(test_add_to_scrollback) {

    Scrollback *sb = create_scrollback(NULL, 5);

    cchar_t line[100+1] = {0};
    cchar_t *linePtr = line;
    
    linePtr += convert_to_cchar(linePtr, "12:00", 0);
    linePtr += convert_to_cchar(linePtr, " ## ", COLOR_SEP(MAGENTA));
    linePtr += convert_to_cchar(linePtr, "Buzz v0.1",  0);
    linePtr += convert_to_cchar(linePtr, "\n", 0);

    add_to_scrollback(sb, line, linePtr - line);

    ck_assert_int_eq(sb->head, 1);
    ck_assert_int_eq(sb->usedSize, 1);

    wchar_t result[100 + 1] = {L'\0'};

    int i = 0;
    while (sb->buffer[0] && sb->buffer[0][i].chars[0]) {
        wcscat(result, sb->buffer[0][i++].chars);
    }

    ck_assert_wstr_eq(L"12:00 ## Buzz v0.1\n", result);

    delete_scrollback(sb);
}
END_TEST

Suite* scrollback_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Scrollback");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_sb);
    tcase_add_test(tc_core, test_isempty_sb);
    tcase_add_test(tc_core, test_isfull_sb);
    tcase_add_test(tc_core, test_get_preceding_ln_count);
    tcase_add_test(tc_core, test_convert_to_cchar);
    tcase_add_test(tc_core, test_add_to_scrollback);

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
