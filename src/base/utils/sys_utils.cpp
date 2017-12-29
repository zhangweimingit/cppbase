#include <sys/prctl.h>

#include "base/utils/ik_logger.h"
#include "base/utils/sys_utils.hpp"
#include <sstream>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

namespace cppbase {

bool SysCmd::get_result(string &res) throw (Errno)
{
	if (pipe(pipefd_) == -1) {
		throw Errno("Fail to pipe");
	}

	sighandler_t old_handler;

	pid_t child_pid = fork();
	if (child_pid == -1) {
		throw Errno("Fail to fork");
	}

	old_handler = signal(SIGCHLD, SIG_IGN);
	if (child_pid) {
		unsigned char buf[1024];
	
		::close(pipefd_[1]);
		pipefd_[1] = -1;

		stringstream ss;
		fd_set rds;
		struct timeval timeout;
		bool result = true;

		FD_ZERO(&rds);
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = timeout_secs_;

		int fd = pipefd_[0];
		while (1) {
			int ret;

			FD_SET(fd, &rds);

			ret = select(fd+1, &rds, NULL, NULL, &timeout);
			if (ret) {
				// The condition should be true always, because only one fd
				if (FD_ISSET(fd, &rds)) {
					ret = read(fd, buf, sizeof(buf)-1);
					if (ret <= 0) {
						break;
					}
					buf[ret] = '\0';

					ss << buf;

					if (max_result_bytes_ != SYS_CMD_INFINITE_RET_BYTES &&
						ss.tellp() > max_result_bytes_) {
						/* Set errno is 0, to make sure no system error */
						errno = 0;
						res = "Exceed the result buffer limit:";
						result = false;
						//LOG_ERRO("the result exceeds limit bytes (%u)", max_result_bytes_);
						break;
					}
				}
			} else if (ret == 0) {
				ss << "Timeout(" << timeout_secs_ << " secs) to get result.";
				result = false;
				break;
			} else {
				ss << "Fail to get result:" << strerror(errno);
				// LOG_ERROR << "Fail to get result(" << cmd_ << "): " << strerror(errno) << endl;
				result = false;
				break;
			}
		}

		res.append(ss.str());
		signal(SIGCHLD, old_handler);
		return result;
	} else {
		prctl(PR_SET_NAME, "SysCmdFork1",NULL,NULL,NULL);
		signal(SIGCHLD, old_handler);
		pid_t pid = fork();
		if (pid) {
			//Now init would clean up it.
			_Exit(0);
		}

		prctl(PR_SET_NAME, "SysCmdFork1",NULL,NULL,NULL);
		::close(pipefd_[0]);
		if (dup2(pipefd_[1], 1) == -1) {
			//LOG_ERRO("Fail to dup2 for cmd %s", cmd_.c_str());
			_Exit(-1);
		}
		::close(pipefd_[1]);
		execve(cmd_.c_str(), &argv_[0], &env_[0]);
		_Exit(-1);
	}
}

void SysTimerFd::start(void) throw (Errno)
{	
	fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
	if (fd_ == -1) {
		throw Errno("Fail to timerfd_create");
	}

	set_interval();
	start_ = true;
}

void SysTimerFd::get_expired_cnt(uint64_t &expired_cnt) throw (Errno)
{	
	ssize_t ret;

	ret = read(fd_, &expired_cnt, sizeof(expired_cnt));
	if (ret == -1) {
		throw Errno("Fail to read timerfd");
	}
}

bool SysTimerFd::set_timer_interval(uint64_t interval) throw (Errno)
{
	if (!period_ && start_) {
		// LOG_ERROR << "Not support reset pending oneshot timer" << endl;
		return false;
	}
	nsecs_ = interval;

	set_interval();

	return true;
}

void SysTimerFd::set_interval(void) throw (Errno)
{	
	struct itimerspec timeout;
	struct timespec now;
	time_t secs;
	long   nsecs;
	
	memset(&timeout, 0, sizeof(timeout));
	memset(&now, 0, sizeof(now));
	
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
		throw Errno("Fail to clock_gettime");
	}

	secs = nsecs_/1000000000;
	nsecs = nsecs_%1000000000;

	timeout.it_value.tv_sec = now.tv_sec + secs;
	timeout.it_value.tv_nsec = now.tv_nsec + nsecs;

	if (period_) {
		timeout.it_interval.tv_sec = secs;
		timeout.it_interval.tv_nsec = nsecs;
	}

	// LOG_INFO << "Timer secs(" << secs << ") nsecs(" << nsecs << ")" << endl;
	if (timerfd_settime(fd_, TFD_TIMER_ABSTIME, &timeout, NULL) == -1) {
		throw Errno("Fail to timerfd_settime");
	}
	// LOG_INFO << "Timer(" << nsecs_ << " nsecs) is set successfully" << endl;	
}

}
