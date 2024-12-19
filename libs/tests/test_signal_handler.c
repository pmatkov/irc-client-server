#include "../src/signal_handler.h"
#include "../src/common.h"
#include "../src/io_utils.h"
#include "../src/time_utils.h"

#include <check.h>
#include <signal.h>
#include <unistd.h>

START_TEST(test_handle_client_sigint) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    StreamPipe *streamPipe = create_pipe();
    set_client_pipe_fd(get_pipe_fd(streamPipe, WRITE_PIPE));

    set_sigaction(handle_client_sigint, SIGINT, (int[]){SIGINT, 0});

    raise(SIGINT);

    int readStatus = read_message(get_pipe_fd(streamPipe, READ_PIPE), buffer, sizeof(buffer));

    ck_assert_int_eq(readStatus, 1);
    ck_assert_str_eq(buffer, "sigint");

    delete_pipe(streamPipe);

}
END_TEST


START_TEST(test_handle_sigalrm) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    StreamPipe *streamPipe = create_pipe();
    set_client_pipe_fd(get_pipe_fd(streamPipe, WRITE_PIPE));
    
    set_sigaction(handle_sigalrm, SIGALRM, NULL);

    struct itimerval *timer = create_interval_timer(1);

    sleep(2);

    int readStatus = read_message(get_pipe_fd(streamPipe, READ_PIPE), buffer, sizeof(buffer));

    ck_assert_int_eq(readStatus, 1);
    ck_assert_str_eq(buffer, "sigalrm");

    delete_interval_timer(timer);

    delete_pipe(streamPipe);


}
END_TEST


Suite* signal_handler_suite(void) {
    Suite *s;
    TCase *tc_core, *tc_convert;

    s = suite_create("Signal handler");
    tc_core = tcase_create("Core");
    tc_convert = tcase_create("Convert");

    // Add the test case to the test suite
    tcase_add_test(tc_convert, test_handle_client_sigint);
    tcase_add_test(tc_convert, test_handle_sigalrm);

    suite_add_tcase(s, tc_convert);
    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = signal_handler_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
