#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

#include <iostream>

// #include "base/utils/logger.hpp"
#include "core/event/event_poll.hpp"

using namespace std;

namespace cppbase {

bool EventPoll::flags_sanity_check(uint32_t flags)
{	
	if (-1 == epoll_fd_) {
		cerr << "epoll is not init successfully" << endl;
		return false;
	}

	if (!(flags & EPOLL_ALL_FLAGS)) {
		cerr << "Not specify permit flags" << endl;
		return false;
	}

	if (flags & (~EPOLL_ALL_FLAGS)) {
		cerr << "There are non-permit flags" << endl;
		return false;
	}

	return true;
}

void EventPoll::convert_epoll_flags(struct epoll_event & event, int fd, uint32_t flags)
{	
	memset(&event, 0, sizeof(event));
	if (flags & EPOLL_EPOLLIN) {
		event.events |= EPOLLIN;
	}
	if (flags & EPOLL_EPOLLOUT) {
		event.events |= EPOLLOUT;
	}
	if (flags & EPOLL_EPOLLRDHUP) {
		event.events |= EPOLLRDHUP;
	}
	event.data.fd = fd;
}


bool EventPoll::init(void)
{
	epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
	if (-1 == epoll_fd_) {
		cerr << "Fail to create epoll: " << strerror(errno) << endl;
		return false;
	}

	return (epoll_fd_ != -1);
}

bool EventPoll::epoll_add_fd(int fd, unsigned int flags)
{
	if (!flags_sanity_check(flags)) {
		return false;
	}
	
	struct epoll_event event;
	convert_epoll_flags(event, fd, flags);

	int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
	if (0 != ret) {
		cerr << "epoll_ctl failed: " << strerror(errno) << endl;
		return false;
	}
	// LOG_DEBUG << "Add fd " << fd << " into epoll successfully" << endl;

	mon_fd_cnts_++;
	ready_events_.push_back(event);

	return true;
}

bool EventPoll::epoll_modify_fd(int fd,unsigned int flags)
{
	if (!flags_sanity_check(flags)) {
		return false;
	}
	
	struct epoll_event event;
	convert_epoll_flags(event, fd, flags);

	int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
	if (0 != ret) {
		cerr << "epoll_ctl failed: " << strerror(errno) << endl;
		return false;
	}
	// LOG_DEBUG << "Modify fd " << fd << " into epoll successfully" << endl;

	return true;	
}

void EventPoll::epoll_del_fd(int fd)
{
	if (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL)) {
		mon_fd_cnts_--;
	}
	
	if (ready_events_.size() >= mon_fd_cnts_+EPOOL_EVENT_MAX_WASTE_CNT) {
		ready_events_.reserve(mon_fd_cnts_+EPOOL_EVENT_MAX_WASTE_CNT);
	}
}

uint32_t EventPoll::epoll_wait(std::vector<EPEvent> &ready_fds, int wait_secs)
{
	if (-1 != wait_secs) {
		wait_secs *= 1000;
	}

	int ret = ::epoll_wait(epoll_fd_, &ready_events_[0], ready_events_.size(), wait_secs);
	if (-1 == ret) {
		// LOG_DEBUG << "epoll_wait failed " << strerror(errno) << endl;
		if (errno != EINTR) {
			// LOG_ERROR << "epoll_wait failed " << strerror(errno) << endl;
			cerr << "epoll_wait failed: " << strerror(errno) << endl;
		}
		ready_fds.resize(0);
		return 0;
	} else if (0 == ret) {
		ready_fds.resize(0);
		return 0;
	}

	if ((uint32_t)ret > ready_fds.capacity()) {
		ready_fds.reserve(ret);
	}

	for (int i = 0; i < ret; ++i) {
		EPEvent e(ready_events_[i].data.fd, ready_events_[i].events);

		ready_fds[i] = e;
	}
	ready_fds.resize(ret);

	// LOG_DEBUG << "epoll_wait finds " << ret << " ready fds" << endl;
	return ret;
}

uint32_t EventPoll::epoll_wait(std::vector<EPEvent> &ready_fds)
{
	return epoll_wait(ready_fds, -1);
}

}



