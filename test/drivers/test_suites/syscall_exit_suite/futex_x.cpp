#include "../../event_class/event_class.h"

#include <linux/futex.h>
#include <stdint.h>
#include <sys/time.h>

#ifdef __NR_futex
TEST(SyscallEnter, futexX) {
	auto evt_test = get_syscall_event_test(__NR_futex, EXIT_EVENT);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL ===========================*/

	uint32_t futex_word;
	int futex_op = FUTEX_PRIVATE_FLAG;
	uint32_t val = 7;
	assert_syscall_state(SYSCALL_FAILURE,
	                     "futex",
	                     syscall(__NR_futex, &futex_word, futex_op, val, NULL, NULL, 0));
	int64_t errno_value = -errno;

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->assert_event_presence();

	if(HasFatalFailure()) {
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)errno_value);

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(1);
}
#endif
