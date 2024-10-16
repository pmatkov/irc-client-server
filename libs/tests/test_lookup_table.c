#include "../src/priv_lookup_table.h"
#include "../src/string_utils.h"

#include <check.h>


enum color {RED, GREEN, BLUE};

static int keys[] = {RED, GREEN, BLUE};
static const char *values[] = {"red", "green", "blue"};

START_TEST(test_create_lookup_table) {

    LookupTable *lookupTable = create_lookup_table(keys, values, ARR_SIZE(keys));

    ck_assert_ptr_ne(lookupTable, NULL);
    ck_assert_int_eq(lookupTable->size, ARR_SIZE(keys));

    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_lookup_value) {

    LookupTable *lookupTable = create_lookup_table(keys, values, ARR_SIZE(keys));

    ck_assert_str_eq(lookup_value(lookupTable, RED), "red");

    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_set_value) {

    LookupTable *lookupTable = create_lookup_table(keys, values, ARR_SIZE(keys));

    set_value(lookupTable, RED, "black");
    
    ck_assert_str_eq(lookup_value(lookupTable, RED), "black");

    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_lookup_key) {

    LookupTable *lookupTable = create_lookup_table(keys, values, ARR_SIZE(keys));

    ck_assert_int_eq(lookup_key(lookupTable, "red"), RED);

    delete_lookup_table(lookupTable);

}
END_TEST

START_TEST(test_get_lookup_pair) {

    LookupTable *lookupTable = create_lookup_table(keys, values, ARR_SIZE(keys));

    Pair *pair = get_lookup_pair(lookupTable, RED);

    ck_assert_ptr_ne(pair, NULL);
    ck_assert_int_eq(get_pair_key(pair), RED);
    ck_assert_str_eq(get_pair_value(pair), "red");

    delete_lookup_table(lookupTable);

}
END_TEST

Suite* lookup_table_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Lookup table");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_lookup_table);
    tcase_add_test(tc_core, test_lookup_value);
    tcase_add_test(tc_core, test_set_value);
    tcase_add_test(tc_core, test_lookup_key);
    tcase_add_test(tc_core, test_get_lookup_pair);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = lookup_table_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
