#include "../src/priv_time_utils.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum Month {JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC, UNKNOWN_MONTH, MONTH_COUNT};

struct MonthMap {
    enum Month month;
    const char *string;
};

static struct MonthMap monthMap[] = {
    {JAN, "Jan"},
    {FEB, "Feb"},
    {MAR, "Mar"},
    {APR, "Apr"},
    {MAY, "May"},
    {JUN, "Jun"},
    {JUL, "Jul"},
    {AUG, "Aug"}, 
    {SEP, "Sep"},
    {OCT, "Oct"},
    {NOV, "Nov"},
    {DEC, "Dec"},
    {UNKNOWN_MONTH, NULL}
};

enum Month string_to_month(const char *string) {

    for (int i = 0; i < MONTH_COUNT; i++) {
        if (strcmp(monthMap[i].string, string) == 0) {
            return monthMap[i].month;
        }
    }
    return UNKNOWN_MONTH;
}

START_TEST(test_get_datetime) {

    char timestamp1[DATE_LENGTH] = {'\0'};
    get_datetime(get_format_function(DATE), timestamp1, DATE_LENGTH);

    char timestamp2[DATE_LENGTH] = {'\0'};
    char month[4];
    int d, m, y;

    sscanf(__DATE__, "%s %d %d", month, &d, &y);
    m = string_to_month(month);
    sprintf(timestamp2, "%04d-%02d-%02d", y, m + 1, d);

    ck_assert_int_ne(strlen(timestamp1), 0);
    ck_assert_str_eq(timestamp1, timestamp2);

}
END_TEST

START_TEST(test_create_timer) {

    Timer *timer = create_timer();

    ck_assert_ptr_ne(timer, NULL);

    delete_timer(timer);

}
END_TEST

START_TEST(test_calculate_elapsed_time) {

    Timer *timer = create_timer();

    start_timer(timer);
    sleep(1);
    stop_timer(timer);

    int elapsedTime = get_elapsed_time(timer, SECONDS);

    ck_assert_int_eq(elapsedTime, 1);

    reset_timer(timer);
    ck_assert_int_eq(timer->startTime.tv_sec, 0);

    delete_timer(timer);

}
END_TEST

START_TEST(test_is_timer_active) {

    Timer *timer = create_timer();

    start_timer(timer);
    stop_timer(timer);

    int active = is_timer_active(timer);
    ck_assert_int_eq(active, 1);

    reset_timer(timer);

    active = is_timer_active(timer);
    ck_assert_int_eq(active, 0);

    delete_timer(timer);

}
END_TEST

START_TEST(test_create_interval_timer) {

    struct itimerval *timer = create_interval_timer(60);

    ck_assert_ptr_ne(timer, NULL);
    ck_assert_int_eq(timer->it_interval.tv_sec, 60);

    delete_interval_timer(timer);

}
END_TEST

Suite* time_utils_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Time utils");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_get_datetime);
    tcase_add_test(tc_core, test_create_timer);
    tcase_add_test(tc_core, test_calculate_elapsed_time);
    tcase_add_test(tc_core, test_is_timer_active);
    tcase_add_test(tc_core, test_create_interval_timer);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = time_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
