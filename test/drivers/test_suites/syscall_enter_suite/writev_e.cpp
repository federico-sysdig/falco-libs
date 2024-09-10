#include "../../event_class/event_class.h"

#ifdef __NR_writev

TEST(SyscallEnter, writevE_empty_iovec) {
	auto evt_test = get_syscall_event_test(__NR_writev, ENTER_EVENT);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL ===========================*/

	int mock_fd = -12;
	iovec* iov = NULL;
	int32_t iovcnt = 7;
	assert_syscall_state(SYSCALL_FAILURE, "writev", syscall(__NR_writev, mock_fd, iov, iovcnt));

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->assert_event_presence();

	if(HasFatalFailure()) {
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: fd (type: PT_FD) */
	evt_test->assert_numeric_param(1, (int64_t)mock_fd);

	/* Parameter 2: size (type: PT_UINT32)*/
	evt_test->assert_numeric_param(2, (uint32_t)0);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}

TEST(SyscallEnter, writevE_full_iovec) {
	auto evt_test = get_syscall_event_test(__NR_writev, ENTER_EVENT);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL ===========================*/

	int mock_fd = -12;
	struct iovec iov[2];
	memset(iov, 0, sizeof(iov));
	iov[0].iov_len = DEFAULT_SNAPLEN;
	iov[1].iov_len = DEFAULT_SNAPLEN + 1;
	int32_t iovcnt = 2;
	assert_syscall_state(SYSCALL_FAILURE, "writev", syscall(__NR_writev, mock_fd, iov, iovcnt));

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->assert_event_presence();

	if(HasFatalFailure()) {
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: fd (type: PT_FD) */
	evt_test->assert_numeric_param(1, (int64_t)mock_fd);

	/* Parameter 2: size (type: PT_UINT32)*/
	evt_test->assert_numeric_param(2, (uint32_t)DEFAULT_SNAPLEN * 2 + 1);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(2);
}
#endif
