#ifndef TASK_SERVER_HPP_
#define TASK_SERVER_HPP_

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>
#include <set>

#include "core/thread/thread.hpp"
#include "core/net/socket.hpp"
#include "core/event/event_poll.hpp"
#include "core/net/conn.hpp"
#include "base/utils/errno.hpp"
#include "base/utils/sys_utils.hpp"
#include "base/utils/networks_utils.hpp"

namespace cppbase {

typedef std::function<bool (void)> ExitCallback;
class TaskServer {
public:
	virtual bool init(void) = 0;
	virtual void start(void *data) = 0;

	void set_exit_callback(const ExitCallback &cb) {
		exit_cb_ = cb;
	}
	ExitCallback get_exit_callback(void) const {
		return exit_cb_;
	}
	bool exit(void) const {
		return (exit_cb_ && exit_cb_());
	}
private:
	ExitCallback exit_cb_;
};

typedef std::shared_ptr<TaskServer> TaskServerPtr;

class ServerSet {
public:
	virtual bool init(void) = 0;
	virtual void start(void *data) = 0;

	template <typename T>
	bool init_servers(const char * server_name, uint32_t cnt, uint32_t ip, uint16_t port, void *data, ExitCallback exit_cb = NULL, bool detach_server = false) {
		for (uint32_t i = 0; i < cnt; ++i) {
			char name[32];
			snprintf(name, sizeof(name), "%s%u", server_name, i);
			auto server = std::make_shared<T>(i, ip, port, data);
			ThreadPtr thread = std::make_shared<Thread>(std::bind(&T::start, server, std::placeholders::_1), data, name, detach_server);
			if (exit_cb) {
				server->set_exit_callback(exit_cb);
			}
			if (server->init()) {
				servers_.push_back(server);
				thr_pool_.append_thread(thread);
			} else {
				// std::cerr << name << " fail to init" << std::endl;
				return false;
			}
		}
		return true;
	}

	template <typename T>
	bool init_servers(const char * server_name, uint32_t cnt, std::string ip, uint16_t port, void *data, ExitCallback exit_cb = NULL, bool detach_server = false) {
		return init_servers<T> (server_name, cnt, convert_str_to_ipv4(ip), port, data, exit_cb, detach_server);
	}

	void start_servers(void) {
		thr_pool_.start_all_threads();
	}

	void wait_servers_stoped(void) {
		thr_pool_.wait_all_threads_stoped();
	}

private:
	std::vector<TaskServerPtr> servers_;
	ThreadPool thr_pool_;
};

enum ConnEvent {
	CONN_CONNECTED,
	CONN_DISCONNECTED,
};
typedef std::function<void (const ConnPtr &conn, ConnEvent event) > ConnCallback;
typedef std::function<void (const ConnPtr &conn, PacketBufPtr &msg) > MsgCallback;
typedef std::function<void (const int signum) > SignalCallback;
typedef std::function<void (uint64_t expired_cnt, void *data) > PeriodTimerCallback;
typedef std::function<void (void *data) > OneshotTimerCallback;

class UDPServer: public TaskServer {
public:
	UDPServer(uint32_t ip, uint16_t port): ip_(ip), port_(port), msg_cb_(NULL) {
	}
	void set_msg_callback(const MsgCallback &cb)
	{
		msg_cb_ = cb;
	}
	bool init(void);
	void start(void *data);
private:
	uint32_t ip_;
	uint16_t port_;
	MsgCallback msg_cb_;

	Socket sock_;
	PacketBufPtr msg_buf_;
};

class TCPServer: public TaskServer {
public:
	struct OneshotTimer {
		OneshotTimer(uint64_t nsecs) {
			timerfd_ = std::make_shared<SysTimerFd> (nsecs);
		}
		SysTimerFdPtr timerfd_;
		OneshotTimerCallback cb_;
		void *data_;
	};
	typedef std::shared_ptr<OneshotTimer> OneshotTimerPtr;


	TCPServer(uint32_t ip, uint16_t port): ip_(ip), port_(port), conn_cb_(NULL), msg_cb_(NULL) {
		sig_fd_ = -1;
	}

	TCPServer(const std::string& ip, uint16_t port)
		: TCPServer(convert_str_to_ipv4(ip), port)
	{}

	~TCPServer() {
		if (sig_fd_ != -1) {
			close(sig_fd_);
		}
	}

	void set_conn_callback(const ConnCallback &cb) {
		conn_cb_ = cb;
	}
	void set_msg_callback(const MsgCallback &cb) {
		msg_cb_ = cb;
	}
	void set_signal_callback(const SignalCallback &cb) {
		sig_cb_ = cb;
	}
	void set_period_timer_callback(const PeriodTimerCallback &cb, void *data) {
		period_timer_cb_ = cb;
		period_timer_data_ = data;
	}
	void add_oneshot_timer(const OneshotTimerCallback &cb, uint64_t nsecs, void *data) throw (Errno);
	void add_signal(int signo) {
		signals_.insert(signo);
	}

	void set_period_timer_interval(uint64_t period_ms) {
		if (period_timer_) {
			period_timer_->set_timer_interval(period_ms*1000000);
		} else {
			period_timer_ = std::make_shared<SysTimerFd> (period_ms*1000000, true);
		}
	}

	void close_conn(ConnPtr &conn)
	{
		if (!conn->send_buf_empty()) {
			wait_write_conns_.erase(conn);
		}
		wait_read_conns_.erase(conn);

		if (conn->send_buf_empty() || conn->is_force_close()) {
			epoll_.epoll_del_fd(conn->get_fd());
			conns_.erase(conn->get_fd());
			conn->close();
		} else {
			conn->set_remote_fin();
		}
	}
	bool init(void);
	void start(void *data) throw (Errno);

private:
	void accept_new_conn(void);
	void conn_read_data(int fd);
	void conn_write_data(int fd);
	void process_msgs(void);
	void create_signal_fd(void) throw (Errno);
	void process_signals(void) throw (Errno);
	void create_timer_fd(void) throw (Errno);
	void process_timer(void) throw (Errno);
	OneshotTimerPtr find_oneshot_timer(int fd) const;
	void process_oneshot_timer(OneshotTimerPtr &timer);
	void add_conn_wait_write(const ConnPtr &conn);
	void remove_conn_wait_write(const ConnPtr &conn);
	uint32_t ip_;
	uint16_t port_;
	ConnCallback conn_cb_;
	MsgCallback msg_cb_;
	SignalCallback sig_cb_;
	PeriodTimerCallback period_timer_cb_;
	std::set<int> signals_;
	Socket lsock_;
	EventPoll epoll_;
	int sig_fd_;
	SysTimerFdPtr period_timer_;
	void *period_timer_data_;
	std::map<int, ConnPtr> conns_;
	std::set<ConnPtr> wait_read_conns_;
	std::set<ConnPtr> wait_write_conns_;
	std::map<int, OneshotTimerPtr> oneshot_timers_;
};
typedef std::shared_ptr<TCPServer> TCPServerPtr;
} // namespace cppbase
#endif

