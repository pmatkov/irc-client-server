#include "../src/priv_queue.h"
#include "../src/priv_message.h"

#include <check.h>

#define QUEUE_CAPACITY 3

START_TEST(test_create_queue) {

    Queue *queue = create_queue(QUEUE_CAPACITY, sizeof(RegMessage));

    ck_assert_ptr_ne(queue, NULL);
    ck_assert_int_eq(queue->itemSize, sizeof(RegMessage));
    ck_assert_int_eq(queue->front, 0);
    ck_assert_int_eq(queue->rear, 0);
    ck_assert_int_eq(queue->currentItem, 0);
    ck_assert_int_eq(queue->capacity, QUEUE_CAPACITY);
    ck_assert_int_eq(queue->count, 0);

    delete_queue(queue);
}
END_TEST

START_TEST(test_is_queue_empty) {

    Queue *queue = create_queue(QUEUE_CAPACITY, sizeof(RegMessage));

    ck_assert_int_eq(is_queue_empty(queue), 1);

    delete_queue(queue); 
}
END_TEST

START_TEST(test_is_queue_full) {

    Queue *queue = create_queue(QUEUE_CAPACITY, sizeof(RegMessage));

    enqueue(queue, &(ExtMessage){"john", "steven", "message1"});
    enqueue(queue, &(ExtMessage){"john", "mark", "message2"});
    enqueue(queue, &(ExtMessage){"john", "steven", "message3"});

    ck_assert_int_eq(is_queue_full(queue), 1); 

    delete_queue(queue);
}
END_TEST

START_TEST(test_enqueue) {

    Queue *queue = create_queue(QUEUE_CAPACITY, sizeof(ExtMessage));

    enqueue(queue, &(ExtMessage){"john", "steven", "message1"});

    ExtMessage *message = get_previous_item(queue);

    ck_assert_str_eq(message->sender, "john");
    ck_assert_str_eq(message->recipient, "steven");
    ck_assert_str_eq(message->content, "message1");

    ck_assert_int_eq(queue->rear, 1);
    ck_assert_int_eq(queue->front, 0);
    ck_assert_int_eq(queue->count, 1);

    enqueue(queue, &(ExtMessage){"john", "mark", "message2"});
    enqueue(queue, &(ExtMessage){"john", "steven", "message3"});

    ck_assert_int_eq(queue->rear, 0);
    ck_assert_int_eq(queue->front, 0);
    ck_assert_int_eq(queue->count, 3);

    enqueue(queue, &(ExtMessage){"john", "steven", "message4"});

    ck_assert_int_eq(queue->rear, 1);
    ck_assert_int_eq(queue->front, 1);
    ck_assert_int_eq(queue->count, 3);

    delete_queue(queue); 
}
END_TEST

START_TEST(test_dequeue) {

    Queue *queue = create_queue(QUEUE_CAPACITY, sizeof(ExtMessage));

    enqueue(queue, &(ExtMessage){"john", "steven", "message1"});
    enqueue(queue, &(ExtMessage){"john", "mark", "message2"});
    enqueue(queue, &(ExtMessage){"john", "steven", "message3"});

    ck_assert_int_eq(queue->rear, 0);
    ck_assert_int_eq(queue->front, 0);
    ck_assert_int_eq(queue->count, 3);

    ExtMessage *message = dequeue(queue);

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message->sender, "john");
    ck_assert_str_eq(message->recipient, "steven");
    ck_assert_str_eq(message->content, "message1");

    ck_assert_int_eq(queue->rear, 0);
    ck_assert_int_eq(queue->front, 1);
    ck_assert_int_eq(queue->count, 2);

    dequeue(queue);
    dequeue(queue);

    ck_assert_int_eq(queue->count, 0);

    delete_queue(queue); 
}
END_TEST

START_TEST(test_get_item) {

    Queue *queue = create_queue(QUEUE_CAPACITY, sizeof(RegMessage));
    
    enqueue(queue, &(RegMessage){"message1"});
    enqueue(queue, &(RegMessage){"message2"});

    RegMessage *message = get_previous_item(queue);

    ck_assert_str_eq(message->content, "message2");

    message = get_previous_item(queue);
    ck_assert_str_eq(message->content, "message1");

    message = get_previous_item(queue);
    ck_assert_ptr_eq(message, NULL);

    message = get_current_item(queue);
    ck_assert_str_eq(message->content, "message1");

    message = get_next_item(queue);
    ck_assert_str_eq(message->content, "message2");

    delete_queue(queue);
}
END_TEST


Suite* queue_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Queue");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_queue);
    tcase_add_test(tc_core, test_is_queue_empty);
    tcase_add_test(tc_core, test_is_queue_full);
    tcase_add_test(tc_core, test_enqueue);
    tcase_add_test(tc_core, test_dequeue);
    tcase_add_test(tc_core, test_get_item);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = queue_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
