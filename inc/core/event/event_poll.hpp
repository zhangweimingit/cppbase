#ifndef EVENT_POLL_HPP_
#define EVENT_POLL_HPP_

#include <sys/epoll.h>
#include <unistd.h>

#include <vector>
#include <utility>

namespace cppbase {

class EventPoll {
public:
	struct EPEvent {
		EPEvent() {}

		EPEvent(int fd, uint32_t events): fd_(fd), events_(events) {
		}
	
		int fd_;
		uint32_t events_;
	};

	EventPoll(): epoll_fd_(-1), mon_fd_cnts_(0) {
	}
	~EventPoll() {
		if (-1 != epoll_fd_) {
			close(epoll_fd_);
		}
	}

	enum {
		EPOLL_EPOLLIN = EPOLLIN,
		EPOLL_EPOLLOUT = EPOLLOUT,
		EPOLL_EPOLLRDHUP = EPOLLRDHUP,

		EPOLL_ALL_FLAGS = (EPOLL_EPOLLIN|EPOLL_EPOLLOUT|EPOLL_EPOLLRDHUP),
	};
	
	bool init(void);
	bool epoll_add_fd(int fd, unsigned int flags);
	bool epoll_modify_fd(int fd, unsigned int flags);
	void epoll_del_fd(int fd);
	uint32_t epoll_wait(std::vector<EPEvent> &ready_fds, int wait_secs);
	uint32_t epoll_wait(std::vector<EPEvent> &ready_fds);
	
private:
	bool flags_sanity_check(uint32_t flags);
	void convert_epoll_flags(struct epoll_event &event, int fd, uint32_t flags);

	int epoll_fd_;
	enum {
		EPOOL_EVENT_MAX_WASTE_CNT = 48,
	};
	std::vector<struct epoll_event> ready_events_;
	uint32_t mon_fd_cnts_;
};

}

#endif

