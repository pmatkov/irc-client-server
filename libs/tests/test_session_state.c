#include "../src/session_state.h"
#include "../src/common.h"
#include "../src/command.h"

#include <check.h>

START_TEST(test_is_valid_state_transition) {

    const SessionState **sessionStates = get_client_session_states();

    ck_assert_int_eq(is_allowed_state_transition(sessionStates, DISCONNECTED, CONNECTED), 1);
    ck_assert_int_eq(is_allowed_state_transition(sessionStates, IN_CHANNEL, DISCONNECTED), 1);
    ck_assert_int_eq(is_allowed_state_transition(sessionStates, IN_CHANNEL, REGISTERED), 1);
    ck_assert_int_eq(is_allowed_state_transition(sessionStates, DISCONNECTED, START_REGISTRATION), 0);
    ck_assert_int_eq(is_allowed_state_transition(sessionStates, REGISTERED, UNKNOWN_SESSION_STATE_TYPE), 0);

}
END_TEST

START_TEST(test_is_allowed_state_command) {

    const SessionState **sessionStates = get_client_session_states();

    ck_assert_int_eq(is_allowed_state_command(sessionStates, CONNECTED, NICK), 1);
    ck_assert_int_eq(is_allowed_state_command(sessionStates, REGISTERED, WHOIS), 1);
    ck_assert_int_eq(is_allowed_state_command(sessionStates, DISCONNECTED, START_REGISTRATION), 0);
}
END_TEST

START_TEST(test_get_connection_state) {

    const SessionState **sessionStates = get_client_session_states();
    const SessionState *sessionState = get_session_state(sessionStates, CONNECTED);
    ck_assert_int_eq(sessionState->initialState, CONNECTED);

    sessionState = get_session_state(sessionStates, 10);
    ck_assert_ptr_eq(sessionState, NULL);

}
END_TEST

Suite* session_state_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Session state");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_is_valid_state_transition);
    tcase_add_test(tc_core, test_is_allowed_state_command);
    tcase_add_test(tc_core, test_get_connection_state);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = session_state_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
