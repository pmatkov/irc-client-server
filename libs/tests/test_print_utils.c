#include "../src/print_utils.h"
#include "../src/common.h"
#include "../src/error_control.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define ck_assert_wstr_eq(s1, s2) \
    ck_assert_msg((wcscmp((s1), (s2)) == 0), \
                  "Failed: '%ls' != '%ls'", \
                  s1, s2)

START_TEST(test_compress_bits) {

    int result = 0;

    result = encode_text_format(0, 3, FORMAT_MASK_SEP);
    ck_assert_int_eq(result, 0xFF);

    result = encode_text_format(1, 4, FORMAT_MASK_ORG);
    ck_assert_int_eq(result, 0xFF);

    result = encode_text_format(2, 5, FORMAT_MASK_CNT);
    ck_assert_int_eq(result, 0xFF);
}
END_TEST

START_TEST(test_str_to_complex_str) {

    cchar_t buffer[MAX_CHARS + 1] = {0};
    int charsConverted = 0;
    
    charsConverted = str_to_complex_str(buffer, ARRAY_SIZE(buffer), "Buzz v0.1\n x", COLOR_SEP(MAGENTA));

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
    
    charsConverted = str_to_complex_str(buffer, ARRAY_SIZE(buffer), "Buzz v0.1\n x", COLOR_SEP(MAGENTA));

    wchar_t wideString[MAX_CHARS + 1] = {L'\0'};

    int i = 0;
    while (i < charsConverted && buffer[i].chars[0] != L'\0') {
        wcscat(wideString, buffer[i++].chars);
    }
    int count = count_complex_chars(buffer);

    ck_assert_int_eq(count, strlen("Buzz v0.1\n x"));

}
END_TEST

Suite* print_utils_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Print utils");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_compress_bits);
    tcase_add_test(tc_core, test_str_to_complex_str);
    tcase_add_test(tc_core, test_count_complex_chars);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = print_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
