#include "../src/network_utils.h"
#include "../src/string_utils.h"

#include <check.h>
#include <unistd.h>
#include <sys/wait.h>

START_TEST(test_hostname_to_ip) {

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    int status = hostname_to_ip(ipv4Address, sizeof(ipv4Address), "localhost");
    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(ipv4Address, "127.0.0.1");

    status = hostname_to_ip(ipv4Address, sizeof(ipv4Address), "127.0.0.1");
    ck_assert_int_eq(status, 1);
    
    status = hostname_to_ip(ipv4Address, sizeof(ipv4Address), "local");
    ck_assert_int_eq(status, 0);

}
END_TEST

START_TEST(test_ip_to_hostname) {

    char hostname[MAX_CHARS + 1] = {'\0'};

    int status = ip_to_hostname(hostname, sizeof(hostname), "127.0.0.1");
    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(hostname, "localhost");

    memset(hostname, '\0', sizeof(hostname));

    status = ip_to_hostname(hostname, sizeof(hostname), "127");
    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(hostname, "");

    status = ip_to_hostname(hostname, sizeof(hostname), "localhost");
    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(hostname, "");

}
END_TEST


START_TEST(test_get_local_address) {

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
    int port;

    int listenFd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50105); 
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    bind(listenFd, (struct sockaddr *) &sa, sizeof(sa));
    get_local_address(ipv4Address, INET_ADDRSTRLEN, &port, listenFd);

    ck_assert_str_eq(ipv4Address, "127.0.0.1");
    ck_assert_int_eq(port, 50105);

    close(listenFd);
}
END_TEST


START_TEST(test_get_peer_address) {

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
    int port;

    int listenFd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sa;
    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in)); 
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(50105);

    bind(listenFd, (struct sockaddr *) &sa, sizeof(sa));
    listen(listenFd, 1);

    if (fork() == 0) {

        close(listenFd);

        int clientFd = socket(AF_INET, SOCK_STREAM, 0); 

        struct sockaddr_in servaddr;

        memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(50105);

        inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

        connect(clientFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));
        close(clientFd);
    
    }

    struct sockaddr_in clientSa;
    socklen_t clientSaLen = sizeof(clientSa);
    int connectFd = accept(listenFd, (struct sockaddr*) &clientSa, &clientSaLen);

    get_peer_address(ipv4Address, INET_ADDRSTRLEN, &port, connectFd);
    ck_assert_str_eq(ipv4Address, "127.0.0.1");
    ck_assert_int_ne(port, 50105);

    wait(NULL);

    close(listenFd);
    close(connectFd);

}
END_TEST

START_TEST(test_is_valid_ip) {

    ck_assert_int_eq(is_valid_ip("127.0.0.1"), 1);
    ck_assert_int_eq(is_valid_ip("127"), 0);
    ck_assert_int_eq(is_valid_ip("127.0.0.1."), 0);
    ck_assert_int_eq(is_valid_ip("256.0.0.1"), 0);
}
END_TEST

START_TEST(test_is_valid_port) {

    ck_assert_int_eq(is_valid_port(50100), 1);
    ck_assert_int_eq(is_valid_port(65536), 0);
    ck_assert_int_eq(is_valid_port(123), 0);
}
END_TEST

Suite* network_utils_suite(void) {
    Suite *s;
    TCase *tc_core, *tc_convert;

    s = suite_create("Network utils");
    tc_core = tcase_create("Core");
    tc_convert = tcase_create("Convert");

    // Add the test case to the test suite
    tcase_add_test(tc_convert, test_hostname_to_ip);
    tcase_add_test(tc_convert, test_ip_to_hostname);
    tcase_add_test(tc_convert, test_get_local_address);
    tcase_add_test(tc_convert, test_get_peer_address);
    tcase_add_test(tc_core, test_is_valid_ip);
    tcase_add_test(tc_core, test_is_valid_port);

    suite_add_tcase(s, tc_convert);
    suite_add_tcase(s, tc_core);

    tcase_set_timeout(tc_convert, 15);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = network_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
