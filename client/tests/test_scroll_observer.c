#include "../src/priv_scroll_subject.h"
#include "../src/priv_scroll_observer.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/data_type.h"

#include <stdio.h>
#include <check.h>


static void notify(void *self, const char *message) {

    ScrollObserver *observer = self;

    if (observer != NULL) {
        printf("%s\n", message);
    }
}

START_TEST(test_create_scroll_subject) {

    ScrollSubject *subject = create_scroll_subject(1);

    ck_assert_ptr_ne(subject, NULL);
    ck_assert_ptr_ne(subject->observers, NULL);
    ck_assert_int_eq(subject->count, 0);
    ck_assert_int_eq(subject->capacity, 1);

    delete_scroll_subject(subject);

}
END_TEST

START_TEST(test_create_scroll_observer) {

    ScrollSubject *subject = create_scroll_subject(1);
    ScrollObserver *observer = create_scroll_observer(subject, notify, NULL, NULL);

    ck_assert_ptr_ne(observer, NULL);
    ck_assert_ptr_eq(observer->statusWindow, NULL);
    ck_assert_ptr_eq(observer->inputWindow, NULL);

    ck_assert_int_eq(subject->count, 1);
    ck_assert_ptr_ne(subject->observers[0], NULL);
    ck_assert_ptr_ne(subject->observers[0]->self, NULL);
    ck_assert_ptr_ne(subject->observers[0]->scrollNotifyFunc, NULL);

    delete_scroll_observer(subject, observer);

    ck_assert_int_eq(subject->count, 0);

    delete_scroll_subject(subject);

}
END_TEST

START_TEST(test_notify_scroll_observers) {

    ScrollSubject *subject = create_scroll_subject(1);
    ScrollObserver *observer = create_scroll_observer(subject, notify, NULL, NULL);

    notify_scroll_observers(subject, "new message!");

    delete_scroll_observer(subject, observer);
    delete_scroll_subject(subject);
}
END_TEST


Suite* scroll_observer_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Scroll observer");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_scroll_subject);
    tcase_add_test(tc_core, test_create_scroll_observer);
    tcase_add_test(tc_core, test_notify_scroll_observers);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = scroll_observer_suite();
    sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif


