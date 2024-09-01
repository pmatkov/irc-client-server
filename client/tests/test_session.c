#include "../src/session.h"

#include <check.h>

START_TEST(test_create_session) {

    Session *session = create_session();

    ck_assert_ptr_ne(session, NULL);
    ck_assert_ptr_ne(session_get_inqueue(session), NULL);
    ck_assert_ptr_ne(session_get_outqueue(session), NULL);

    delete_session(session);

}
END_TEST

START_TEST(test_get_set_session_values) {

    Session *session = create_session();
    session_set_fd(session, 2);
    session_set_connected(session, 1);
    session_set_inchannel(session, 1);

    session_set_server_name(session, "server");
    session_set_channel_name(session, "networking");

    ck_assert_int_eq(session_get_fd(session), 2);
    ck_assert_int_eq(session_is_connected(session), 1);
    ck_assert_int_eq(session_is_inchannel(session), 1);
    ck_assert_str_eq(session_get_server_name(session), "server");
    ck_assert_str_eq(session_get_channel_name(session), "networking");

    delete_session(session);
}
END_TEST


Suite* session_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Session");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_session);
    tcase_add_test(tc_core, test_get_set_session_values);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = session_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
