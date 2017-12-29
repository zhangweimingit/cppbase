#ifndef SYS_UTILS_HPP_
#define SYS_UTILS_HPP_

#include <stdio.h>

#include <string>
#include <memory>
#include <vector>

#include "errno.hpp"
#include "ik_logger.h"

namespace cppbase {

class SysCmd {
public:
	enum {
		SYS_CMD_INFINITE_TIMEOUT = -1U,
		SYS_CMD_INFINITE_RET_BYTES = -1U,
	};

	SysCmd(const std::string &command, std::vector<std::string> *argv, std::vector<std::string> *env,
		uint32_t timeout = 120, uint32_t max_bytes = 5 * 1024 * 1024)
		: cmd_(command), timeout_secs_(timeout), max_result_bytes_(max_bytes) {
		argv_.push_back(const_cast<char*>(cmd_.c_str()));
		if (argv) {
			for (auto it = argv->begin(); it != argv->end(); ++it) {
				argv_.push_back(const_cast<char*>(it->c_str()));
			}
		}
		argv_.push_back(NULL);

		if (env) {
			for (auto it = env->begin(); it != env->end(); ++it) {
				env_.push_back(const_cast<char*>(it->c_str()));
			}
		}
		env_.push_back(NULL);
	}
	~SysCmd() {
		close();
	}

	void close() {
		if (pipefd_[0] != -1) {
			::close(pipefd_[0]);
			pipefd_[0] = -1;
		}

		if (pipefd_[1] != -1) {
			::close(pipefd_[1]);
			pipefd_[1] = -1;
		}
	}

	bool get_result(std::string &res) throw (Errno);	
private:
	const std::string cmd_;
	std::vector<char *> argv_;
	std::vector<char *> env_;
	uint32_t timeout_secs_;
	uint32_t max_result_bytes_;
	int pipefd_[2] = {-1, -1};
};

class SysTimerFd {
public:
	SysTimerFd(uint64_t nsecs, bool period = false)
		: nsecs_(nsecs), period_(period), fd_(-1), start_(false)
	{
	}
	void start(void) throw (Errno);
	SysTimerFd() {
		if (fd_ != -1) {
			close(fd_);
		}
	}

	int get_fd(void) const 
	{
		return fd_;
	}

	void get_expired_cnt(uint64_t &expired_cnt) throw (Errno);
	
	/* Don't support running oneshot timer */
	bool set_timer_interval(uint64_t interval) throw (Errno);
private:
	void set_interval(void) throw (Errno);

	uint64_t nsecs_;
	bool period_;
	int fd_;
	bool start_;
};

typedef std::shared_ptr<SysTimerFd> SysTimerFdPtr;

}
#endif

