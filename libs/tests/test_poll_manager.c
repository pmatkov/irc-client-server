#include "../src/priv_poll_manager.h"
#include "../src/common.h"

#include <check.h>
#include <poll.h>

#define POLL_FD 3
#define POLL_FD_CAPACITY 5

START_TEST(test_create_poll_manager) {

    PollManager *pollManager = create_poll_manager(POLL_FD_CAPACITY, POLLIN);

    ck_assert_ptr_ne(pollManager, NULL);
    ck_assert_ptr_ne(pollManager->pfds, NULL);
    ck_assert_int_eq(pollManager->pfds[0].fd, UNASSIGNED);
    ck_assert_int_eq(pollManager->pfds[0].events, POLLIN);
    ck_assert_int_eq(pollManager->count, 0);
    ck_assert_int_eq(pollManager->capacity, POLL_FD_CAPACITY);

    delete_poll_manager(pollManager);
}
END_TEST

START_TEST(test_create_pfd_idx_pair) {

    FdIdxPair *fdIdxPair = create_pfd_idx_pair(POLL_FD, 0);

    ck_assert_ptr_ne(fdIdxPair, NULL);
    ck_assert_int_eq(fdIdxPair->fd, POLL_FD);
    ck_assert_int_eq(fdIdxPair->fdIdx, 0);

    delete_pfd_idx_pair(fdIdxPair);
}
END_TEST

START_TEST(test_are_pfd_idx_equal) {

    FdIdxPair *fdIdxPair1 = create_pfd_idx_pair(POLL_FD, 0);
    FdIdxPair *fdIdxPair2 = create_pfd_idx_pair(POLL_FD + 1, 0);

    ck_assert_int_ne(are_pfd_idx_equal(fdIdxPair1, fdIdxPair2), 1);

    delete_pfd_idx_pair(fdIdxPair1);
    delete_pfd_idx_pair(fdIdxPair2);
}
END_TEST

START_TEST(test_find_poll_fd_idx) {

    PollManager *pollManager = create_poll_manager(POLL_FD_CAPACITY, POLLIN);

    int fdIdx = find_poll_fd_idx(pollManager, UNASSIGNED);

    ck_assert_int_eq(fdIdx, 0);

    delete_poll_manager(pollManager);
}
END_TEST

START_TEST(test_fd_idx_hash_table) {

    PollManager *pollManager = create_poll_manager(POLL_FD_CAPACITY, POLLIN);

    add_fd_idx_to_hash_table(pollManager->fdsIdxMap, POLL_FD, 0);

    int fdIdx = find_fd_idx_in_hash_table(pollManager->fdsIdxMap, POLL_FD);

    ck_assert_int_eq(fdIdx, 0);

    remove_fd_idx_from_hash_table(pollManager->fdsIdxMap, POLL_FD);

    fdIdx = find_fd_idx_in_hash_table(pollManager->fdsIdxMap, POLL_FD);

    ck_assert_int_eq(fdIdx, UNASSIGNED);

    delete_poll_manager(pollManager);
}
END_TEST

START_TEST(test_set_unset_poll_fd) {

    PollManager *pollManager = create_poll_manager(POLL_FD_CAPACITY, POLLIN);

    set_poll_fd(pollManager, POLL_FD);

    int fdIdx = find_fd_idx_in_hash_table(pollManager->fdsIdxMap, POLL_FD);

    ck_assert_int_eq(pollManager->pfds[fdIdx].fd, POLL_FD);
    ck_assert_int_eq(pollManager->count, 1);

    unset_poll_fd(pollManager, POLL_FD);

    ck_assert_int_eq(pollManager->pfds[fdIdx].fd, UNASSIGNED);
    ck_assert_int_eq(pollManager->count, 0);

    delete_poll_manager(pollManager);

}
END_TEST

START_TEST(test_get_poll_data) {

    PollManager *pollManager = create_poll_manager(POLL_FD_CAPACITY, POLLIN);

    ck_assert_ptr_ne(get_poll_pfds(pollManager), NULL);

    set_poll_fd(pollManager, POLL_FD);
    ck_assert_int_eq(get_poll_fd(pollManager, POLL_FD), POLL_FD);

    delete_poll_manager(pollManager);
}
END_TEST

Suite* poll_manager_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Poll manager");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_poll_manager);
    tcase_add_test(tc_core, test_create_pfd_idx_pair);
    tcase_add_test(tc_core, test_are_pfd_idx_equal);
    tcase_add_test(tc_core, test_find_poll_fd_idx);
    tcase_add_test(tc_core, test_fd_idx_hash_table);
    tcase_add_test(tc_core, test_set_unset_poll_fd);
    tcase_add_test(tc_core, test_get_poll_data);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = poll_manager_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
