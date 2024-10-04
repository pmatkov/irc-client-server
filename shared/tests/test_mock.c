#include "../src/mock.h"

#include <check.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CHARS 512

START_TEST(test_mock_read) {

    char input[] = "This is a unit test";
    char buffer[MAX_CHARS + 1] = {'\0'};

    set_mock_buffer(input);
    set_mock_len(sizeof(buffer));
    set_mock_fd(1);

    int bytesRead = mock_read(get_mock_fd(), buffer, strlen(input));

    ck_assert_str_eq(input, buffer);
    ck_assert_int_eq(bytesRead, strlen(input));
}
END_TEST

START_TEST(test_mock_write) {

    char input[] = "This is a unit test";
    char buffer[MAX_CHARS + 1] = {'\0'};

    set_mock_buffer(buffer);
    set_mock_len(sizeof(buffer));
    set_mock_fd(1);

    int bytesWritten = mock_write(get_mock_fd(), input, strlen(input));

    ck_assert_str_eq(input, buffer);
    ck_assert_int_eq(bytesWritten, strlen(input));
}
END_TEST

START_TEST(test_mock_close) {

    set_mock_fd(1);

    int status = mock_close(get_mock_fd());

    ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_mock_socket) {

    set_mock_fd(1);

    int fd = mock_socket(AF_INET, SOCK_STREAM, AF_UNSPEC);

    ck_assert_int_eq(fd, get_mock_fd());
}
END_TEST

START_TEST(test_mock_connect) {

    struct sockaddr_in sa;
    set_mock_fd(1);

    int status = mock_connect(get_mock_fd(), (struct sockaddr *) &sa, 0);

    ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_mock_accept) {

    set_mock_fd(1);
    set_initial_fd_count(get_mock_fd());

    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(sa);

    int fd = mock_accept(get_mock_fd(), (struct sockaddr *)&sa, &sa_len);

    ck_assert_int_eq(fd, get_mock_fd() + 1);
}
END_TEST

START_TEST(test_mock_get_client_ip) {

    struct sockaddr_in sa;
    char buffer[INET_ADDRSTRLEN] = {'\0'};

    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50100);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    set_sockaddr(&sa);

    mock_get_client_ip(buffer, sizeof(buffer), 0);

    ck_assert_str_eq(buffer, "127.0.0.1");
}
END_TEST

Suite* mocks_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Mocks");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_mock_read);
    tcase_add_test(tc_core, test_mock_write);
    tcase_add_test(tc_core, test_mock_close);
    tcase_add_test(tc_core, test_mock_socket);
    tcase_add_test(tc_core, test_mock_connect);
    tcase_add_test(tc_core, test_mock_accept);
    tcase_add_test(tc_core, test_mock_get_client_ip);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = mocks_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
