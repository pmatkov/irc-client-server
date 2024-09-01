#include "../src/test_queue.h"

#include <check.h>

START_TEST(test_create_message_queue) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);

    ck_assert_ptr_ne(mq, NULL);
    ck_assert_int_eq(mq->dataType, REGULAR_MSG);
    ck_assert_int_eq(mq->itemSize, sizeof(RegMessage));
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 0);
    ck_assert_int_eq(mq->allocatedSize, 3);
    ck_assert_int_eq(mq->usedSize, 0);

    delete_message_queue(mq);
}
END_TEST

START_TEST(test_mq_isempty) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);

    ck_assert_int_eq(mq_is_empty(mq), 1);

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_mq_isfull) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);

    ck_assert_int_eq(mq_is_full(mq), 0); 

    delete_message_queue(mq);
}
END_TEST

START_TEST(test_get_char_from_message) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);

    RegMessage message;
    set_reg_message(&message, "test");
    enqueue(mq, &message);

    RegMessage *msg = get_message(mq, 1);

    char ch = get_char_from_message(msg, 1);

    ck_assert_int_eq(ch, 'e');

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_set_char_in_message) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);

    RegMessage message;
    set_reg_message(&message, "");
    enqueue(mq, &message);

    RegMessage *msg = get_message(mq, 1);

    set_char_in_message(msg, 't', strlen(msg->content));
    set_char_in_message(msg, 'x', strlen(msg->content));

    ck_assert_str_eq(msg->content, "tx");

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_set_reg_message) {

    RegMessage message;

    set_reg_message(&message, "test message");

    ck_assert_str_eq(message.content, "test message"); 
}
END_TEST

START_TEST(test_set_ext_message) {

    ExtMessage message;

    set_ext_message(&message, "john", "steven", "test message");

    ck_assert_str_eq(message.sender, "john");
    ck_assert_str_eq(message.recipient, "steven"); 
    ck_assert_str_eq(message.content, "test message"); 
}
END_TEST

START_TEST(test_get_message_content) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);

    RegMessage message;
    set_reg_message(&message, "test");
    enqueue(mq, &message);

    RegMessage *msg = get_message(mq, 1);
    ck_assert_str_eq(get_message_content(msg), "test");

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_enqueue) {

    MessageQueue *mq = create_message_queue(EXTENDED_MSG, 3);

    ExtMessage message1;
    set_ext_message(&message1, "john", "steven", "message 1");
    enqueue(mq, &message1);

    ck_assert_int_eq(mq->usedSize, 1);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 1);

    ExtMessage message2;
    set_ext_message(&message2, "john", "mark", "message 2");
    ExtMessage message3;
    set_ext_message(&message3, "john", "steven", "message 3");
    enqueue(mq, &message2);
    enqueue(mq, &message3);

    ck_assert_int_eq(mq->usedSize, 3);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 0);

    ExtMessage message4;
    set_ext_message(&message4, "john", "steven", "message 4");
    enqueue(mq, &message4);

    ck_assert_int_eq(mq->usedSize, 3);
    ck_assert_int_eq(mq->head, 1);
    ck_assert_int_eq(mq->tail, 1);

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_dequeue) {

    MessageQueue *mq = create_message_queue(EXTENDED_MSG, 3);

    ExtMessage message1;
    set_ext_message(&message1, "john", "steven", "message 1");
    ExtMessage message2;
    set_ext_message(&message2, "john", "mark", "message 2");
    ExtMessage message3;
    set_ext_message(&message3, "john", "steven", "message 3");
    
    enqueue(mq, &message1);
    enqueue(mq, &message2);
    enqueue(mq, &message3);

    ck_assert_int_eq(mq->usedSize, 3);
    ck_assert_int_eq(mq->head, 0);
    ck_assert_int_eq(mq->tail, 0);

    ExtMessage *message = dequeue(mq);

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message->sender, "john");
    ck_assert_str_eq(message->recipient, "steven");
    ck_assert_str_eq(message->content, "message 1");

    ck_assert_int_eq(mq->usedSize, 2);
    ck_assert_int_eq(mq->head, 1);
    ck_assert_int_eq(mq->tail, 0);

    dequeue(mq);
    dequeue(mq);

    ck_assert_int_eq(mq->usedSize, 0);

    delete_message_queue(mq); 
}
END_TEST

START_TEST(test_get_message) {

    MessageQueue *mq = create_message_queue(REGULAR_MSG, 3);
    
    RegMessage message1;
    set_reg_message(&message1, "message 1");
    RegMessage message2;
    set_reg_message(&message2, "message 2");
    enqueue(mq, &message1);
    enqueue(mq, &message2);

    RegMessage *message = get_message(mq, 1);

    ck_assert_str_eq(message->content, "message 2");

    message = get_message(mq, 1);
    ck_assert_str_eq(message->content, "message 1");

    message = get_message(mq, 1);
    ck_assert_ptr_eq(message, NULL);

    message = get_message(mq, 0);
    ck_assert_str_eq(message->content, "message 1");

    message = get_message(mq, -1);
    ck_assert_str_eq(message->content, "message 2");

    delete_message_queue(mq);
}
END_TEST

START_TEST(test_is_valid_data_type) {

    ck_assert_int_eq(is_valid_data_type(REGULAR_MSG), 1);
    ck_assert_int_eq(is_valid_data_type(10), 0);
}
END_TEST

START_TEST(test_get_type_size) {

    ck_assert_int_eq(get_type_size(REGULAR_MSG), sizeof(RegMessage));
}
END_TEST

Suite* queue_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Queue");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_message_queue);
    tcase_add_test(tc_core, test_mq_isempty);
    tcase_add_test(tc_core, test_mq_isfull);
    tcase_add_test(tc_core, test_get_char_from_message);
    tcase_add_test(tc_core, test_set_char_in_message);
    tcase_add_test(tc_core, test_set_reg_message);
    tcase_add_test(tc_core, test_set_ext_message);
    tcase_add_test(tc_core, test_get_message_content);
    tcase_add_test(tc_core, test_enqueue);
    tcase_add_test(tc_core, test_dequeue);
    tcase_add_test(tc_core, test_get_message);
    tcase_add_test(tc_core, test_is_valid_data_type);
    tcase_add_test(tc_core, test_get_type_size);

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
