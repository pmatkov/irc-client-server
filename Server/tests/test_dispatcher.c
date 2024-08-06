#include "../src/dispatcher.h"

#include <check.h>

START_TEST(test_create_message_queue) {

    MessageQueue *mq = create_message_queue(10);

    ck_assert_ptr_ne(mq, NULL);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 0);
    ck_assert_int_eq(mq->allocatedSize, 10);
    ck_assert_int_eq(mq->usedSize, 0);

    delete_message_queue(mq);
}
END_TEST


START_TEST(test_isempty_sb) {

    MessageQueue *mq = create_message_queue(10);

    ck_assert(is_empty(mq));

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_isfull_sb) {

    MessageQueue *mq = create_message_queue(10);
    mq->usedSize = 10;

    ck_assert(is_full(mq)); 

    delete_message_queue(mq);
}
END_TEST

START_TEST(test_set_message) {

    Message message;

    memset(&message, 0, sizeof(message));

    int status = set_message(&message, "john", "steven", "test message");

    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(message.sender, "john");
    ck_assert_str_eq(message.recipient, "steven");
    ck_assert_str_eq(message.content, "test message");

    memset(&message, 0, sizeof(message));

    status = set_message(&message, "toolongnickname", "steven", "message");

    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(message.sender, "");
    ck_assert_str_eq(message.recipient, "");
    ck_assert_str_eq(message.content, "");
}
END_TEST

START_TEST(test_enqueue) {

    MessageQueue *mq = create_message_queue(3);

    enqueue(mq, "john", "steven", "message 1");

    ck_assert_int_eq(mq->usedSize, 1);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 1);

    enqueue(mq, "john", "mark", "message 2");
    enqueue(mq, "john", "steven", "message 3");

    ck_assert_int_eq(mq->usedSize, 3);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 0);

    enqueue(mq, "john", "steven", "message 4");
    ck_assert_int_eq(mq->usedSize, 3);
    ck_assert_int_eq(mq->head, 1);
    ck_assert_int_eq(mq->tail, 1);

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_dequeue) {

    MessageQueue *mq = create_message_queue(3);

    enqueue(mq, "john", "steven", "message 1");
    enqueue(mq, "john", "mark", "message 2");
    enqueue(mq, "john", "steven", "message 3");

    ck_assert_int_eq(mq->usedSize, 3);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 0);

    Message *message = dequeue(mq);

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message->sender, "john");
    ck_assert_str_eq(message->recipient, "steven");
    ck_assert_str_eq(message->content, "message 1");

    ck_assert_int_eq(mq->usedSize, 2);
    ck_assert_int_eq(mq->head, 1);
    ck_assert_int_eq(mq->tail, 0);

    dequeue(mq);
    dequeue(mq);
    message = dequeue(mq);

    ck_assert_ptr_eq(message, NULL);
    ck_assert_int_eq(mq->usedSize, 0);

    delete_message_queue(mq); 
}
END_TEST

Suite* dispatcher_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Dispatcher");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_message_queue);
    tcase_add_test(tc_core, test_isempty_sb);
    tcase_add_test(tc_core, test_isfull_sb);
    tcase_add_test(tc_core, test_set_message);
    tcase_add_test(tc_core, test_enqueue);
    tcase_add_test(tc_core, test_dequeue);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = dispatcher_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif