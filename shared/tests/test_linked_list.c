#include "../src/priv_linked_list.h"

#include <check.h>
#include <stdlib.h>

#define QUEUE_CAPACITY 3
#define MAX_CHARS 512

typedef struct {
    char *name;
    int age;
} Person;

int are_persons_equal(void *person1, void *person2) {
    
    int equal = 0;

    if (person1 != NULL && person2 != NULL) {
        equal = strcmp(((Person*)person1)->name, ((Person*)person2)->name) == 0;
    }

    return equal;
}

Person * create_person(const char *name, int age) {

    Person *person = (Person*) malloc(sizeof(Person));
    person->name = (char*) calloc(MAX_CHARS + 1, sizeof(char));
    strcpy(person->name, name);
    person->age = age;

    return person;
}

void delete_person(void *data) {

    if (data != NULL) {
        free(((Person*)data)->name);
    }

    free(data);
}

START_TEST(test_create_linked_list) {

    LinkedList *linkedList = create_linked_list(are_persons_equal, delete_person);

    ck_assert_ptr_ne(linkedList, NULL);
    ck_assert_ptr_eq(linkedList->head, NULL);
    ck_assert_int_eq(linkedList->count, 0);
    ck_assert_ptr_eq(linkedList->comparatorFunc, are_persons_equal);
    ck_assert_ptr_eq(linkedList->deleteDataFunc, delete_person);

    delete_linked_list(linkedList);
}
END_TEST

START_TEST(test_create_node) {

    Person *person = create_person("john", 35);
    Node *node = create_node(person);

    ck_assert_ptr_ne(node, NULL);
    ck_assert_ptr_ne(node->data, NULL);
    ck_assert_str_eq(((Person*)node->data)->name, "john");
    ck_assert_ptr_eq(node->next, NULL);

    delete_node(node, delete_person);
}
END_TEST

START_TEST(test_append_node) {

    LinkedList *linkedList = create_linked_list(are_persons_equal, delete_person);

    Person *person1 = create_person("john", 35);
    Person *person2 = create_person("mark", 33);

    Node *node1 = create_node(person1);
    Node *node2 = create_node(person2);

    append_node(linkedList, node1);
    append_node(linkedList, node2);

    ck_assert_ptr_eq(linkedList->head, node1);
    ck_assert_str_eq(((Person*)node1->data)->name, "john");
    ck_assert_int_eq(linkedList->count, 2);

    delete_linked_list(linkedList);
}
END_TEST

START_TEST(test_remove_node) {

    LinkedList *linkedList = create_linked_list(are_persons_equal, delete_person);

    Person *person1 = create_person("john", 35);
    Person *person2 = create_person("mark", 33);

    Node *node1 = create_node(person1);
    Node *node2 = create_node(person2);

    append_node(linkedList, node1);
    append_node(linkedList, node2);

    int status = remove_node(linkedList, person1);
    ck_assert_int_eq(status, 1);

    ck_assert_ptr_eq(linkedList->head, node2);
    ck_assert_str_eq(((Person*)linkedList->head->data)->name, "mark");
    ck_assert_int_eq(linkedList->count, 1);

    delete_linked_list(linkedList);
}
END_TEST

START_TEST(test_find_node) {

    LinkedList *linkedList = create_linked_list(are_persons_equal, delete_person);

    Person *person1 = create_person("john", 35);
    Person *person2 = create_person("mark", 33);

    Node *node1 = create_node(person1);
    Node *node2 = create_node(person2);

    append_node(linkedList, node1);
    append_node(linkedList, node2);

    Node *node = find_node(linkedList, person1);

    ck_assert_ptr_ne(node, NULL);
    ck_assert_str_eq(((Person*)node->data)->name, "john");
    ck_assert_int_eq(linkedList->count, 2);

    delete_linked_list(linkedList);
}
END_TEST

Suite* linked_list_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Linked list");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_linked_list);
    tcase_add_test(tc_core, test_create_node);
    tcase_add_test(tc_core, test_append_node);
    tcase_add_test(tc_core, test_remove_node);
    tcase_add_test(tc_core, test_find_node);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = linked_list_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
