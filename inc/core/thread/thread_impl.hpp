#ifndef THREAD_IMPL_HPP_
#define THREAD_IMPL_HPP_

#include <unistd.h>
#include <sys/syscall.h>

namespace cppbase {

class ThreadProperty {
public:
	ThreadProperty() {
		tid_ = ::syscall(SYS_gettid);
	}

	pid_t get_tid() const {
		return tid_;
	}
private:
	pid_t tid_;
};


/* Internally use */
extern thread_local const cppbase::ThreadProperty g_thr_var;

}
#endif

