#include "../src/priv_event.h"

#include <check.h>
#include <stdio.h>

static void print_message(Event *event) {

    printf("%s\n", (char*)event->dataItem.itemChar);
}

START_TEST(test_create_event_manager) {

    EventManager *manager = create_event_manager(0);

    ck_assert_ptr_ne(manager, NULL);
    ck_assert_ptr_ne(manager->eventQueue, NULL);
    ck_assert_int_eq(manager->droppedEvents, 0);
    ck_assert_int_eq(manager->stopProcessing, 0);

    delete_event_manager(manager);
}
END_TEST

START_TEST(test_create_event) {

    Event *event = create_event(UI_EVENT, UI_KEY, (DataItem) {.itemChar = "c"}, CHAR_TYPE);

    ck_assert_ptr_ne(event, NULL);
    ck_assert_int_eq(event->eventType, UI_EVENT);
    ck_assert_int_eq(event->subEventType, UI_KEY);
    ck_assert_str_eq(event->dataItem.itemChar, "c");
    ck_assert_int_eq(event->dataType, CHAR_TYPE);

    delete_event(event);

}
END_TEST

START_TEST(test_register_dispatch_event) {

    EventManager *manager = create_event_manager(0);

    register_base_event_handler(manager, UI_EVENT, print_message);

    ck_assert_ptr_eq(manager->baseHandlers[UI_EVENT], print_message);
    dispatch_base_event(manager, &(Event){
        .eventType = NETWORK_EVENT, 
        .subEventType = NE_CLIENT_MSG, 
        .dataItem = (DataItem){.itemChar = "/privmsg john :hello!"}, 
        .dataType = CHAR_TYPE
    });

    delete_event_manager(manager);
}
END_TEST

Suite* event_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Event");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_event_manager);
    tcase_add_test(tc_core, test_create_event);
    tcase_add_test(tc_core, test_register_dispatch_event);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = event_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
