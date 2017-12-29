#include "base/server/task_server.hpp"
#include "base/utils/ik_logger.h"
#include "base/utils/compiler.hpp"

#include <signal.h>
#include <string.h>
#include <sys/signalfd.h>

#include <iostream>
#include <vector>

using namespace std;

namespace cppbase {

bool TCPServer::init(void)
{
	if (!lsock_.open(AF_INET, SOCK_STREAM, 0, Socket::SOCKET_REUSEADDR_BIT|Socket::SOCKET_REUSEPORT_BIT|Socket::SOCKET_NONBLOCK_BIT)) {
		cerr << "TCPServer fail to open socket" << endl;
		return false;
	}

	if (!lsock_.bind(ip_, port_)) {
		cerr << "TCPServer fail to bind port(" << port_ << ")" << endl;
		return false;
	}
	
	if (!lsock_.listen(5)) {
		cerr << "TCPServer fail to listen port(" << port_ << ")" << endl;
		return false;
	}

	if (!epoll_.init()) {
		cerr << "TCPServer fail to init epoll" << endl;
		return false;
	}
	
	if (!epoll_.epoll_add_fd(lsock_.sock_, EventPoll::EPOLL_EPOLLIN)) {
		cerr << "Fail to add fd into epoll" << endl;
		return false;
	}

	return true;
}

void TCPServer::start(void * data) throw (Errno)
{	
	vector<EventPoll::EPEvent> ready_fds;
	uint32_t ready_cnt;
	bool recv_signal = false;
	bool recv_timer = false;
	OneshotTimerPtr oneshot_timer;

	create_signal_fd();

	create_timer_fd();
	
	while (!exit()) {
		ready_cnt = epoll_.epoll_wait(ready_fds, 1);
		if (!ready_cnt) {
            // LOG_TRAC("no epoll wait event");
			continue;
		}
		
		for (uint32_t i = 0; i < ready_cnt; ++i) {
			if (ready_fds[i].events_ & EventPoll::EPOLL_EPOLLOUT) {
				conn_write_data(ready_fds[i].fd_);
			} else {				
				if (ready_fds[i].fd_ == lsock_.sock_) {
					accept_new_conn();
				} else if (ready_fds[i].fd_ == sig_fd_) {
					recv_signal = true;
                    LOG_TRAC("recv_signal = true");
				} else if (period_timer_ && ready_fds[i].fd_ == period_timer_->get_fd()) {
					recv_timer = true;
                    LOG_TRAC("recv_timer = true");
				} else if ((oneshot_timer = find_oneshot_timer(ready_fds[i].fd_))) {
					process_oneshot_timer(oneshot_timer);
					oneshot_timer.reset();
				} else {
					conn_read_data(ready_fds[i].fd_);
				}
			}		
		}

		if (recv_signal) {
			recv_signal = false;
			process_signals();
		}

		if (likely(msg_cb_)) {
			process_msgs();
		}

		if (recv_timer) {
			recv_timer = false;
			process_timer();
		}
	}
}

void TCPServer::add_oneshot_timer(const OneshotTimerCallback & cb, uint64_t nsecs, void * data) throw (Errno)
{
    LOG_TRAC("begin");
	OneshotTimerPtr timer = make_shared<OneshotTimer> (nsecs);
	timer->cb_ = cb;
	timer->data_ = data;

	timer->timerfd_->start();

	if (!epoll_.epoll_add_fd(timer->timerfd_->get_fd(), EventPoll::EPOLL_EPOLLIN)) {
		throw Errno("Fail to add one-shot timer fd");
	}

	oneshot_timers_.insert(make_pair(timer->timerfd_->get_fd(), timer));
    LOG_TRAC("end");
}

void TCPServer::accept_new_conn(void)
{
    LOG_TRAC("begin");
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	
	int fd = accept4(lsock_.sock_, &addr, &addrlen, SOCK_CLOEXEC);
	if (-1 != fd) { 
		if (!epoll_.epoll_add_fd(fd, EventPoll::EPOLL_EPOLLIN)) {
			LOG_ERRO("Failed to insert new fd into epoll");
			close(fd);
			return;
		}
		
		ConnPtr conn = make_shared<Conn>(fd);
		conn->set_peer_info(Peer::PEER_AF_INET, addr, addrlen);
		conns_[fd] = conn;
	
        LOG_INFO("new conn arrived from: %s", conn->to_str());
		if (conn_cb_) {
			LOG_TRAC("conn_cb_ begin");
			conn_cb_(conn, CONN_CONNECTED);
			LOG_TRAC("conn_cb_ end");
			if (conn->is_force_close()) {
				close_conn(conn);
			} else {
				if (!conn->send_buf_empty() || conn->is_local_fin()) {
					add_conn_wait_write(conn);
				}
			}
		}
	} else {
		LOG_ERRO("Invalid fd(%d) returned by accept, %s", fd, strerror(errno));
        return;
	}
    LOG_TRAC("end");
}

void TCPServer::conn_read_data(int fd)
{
    LOG_TRAC("begin");
	auto fd_conn = conns_.find(fd);
	if (fd_conn == conns_.end()) {
		LOG_ERRO("No connresponding conn");
		return;
	}
	
	ConnPtr conn = fd_conn->second;
	
	bool empty = conn->rcv_buf_empty();
	
	if (!conn->read_bytes()) {
		LOG_INFO("Disconnect the conn: %s", conn->to_str());
		if (likely(conn_cb_)) {
			LOG_TRAC("conn_cb_ begin");
			conn_cb_(conn, CONN_DISCONNECTED);
			LOG_TRAC("conn_cb_ end");
		}
		close_conn(conn);
		return;
	}
	
	if (empty) {
		LOG_DBUG("The conn is ready to read: %s", conn->to_str());
		wait_read_conns_.insert(conn);
	}
    LOG_TRAC("end");
}

void TCPServer::conn_write_data(int fd)
{
    LOG_TRAC("begin");
	auto fd_conn = conns_.find(fd);
	if (fd_conn == conns_.end()) {
		LOG_ERRO("No connresponding conn");
		return;
	}
	
	ConnPtr conn = fd_conn->second;
	conn->send_bytes();
	
	if (conn->send_buf_empty()) {
		remove_conn_wait_write(conn);
		
		// grace close
		if (conn->is_local_fin()) {
			close_conn(conn);
		}
	} else {
		if (conn->is_force_close()) {
			close_conn(conn);
		}
	}

    LOG_TRAC("end");
}

void TCPServer::process_msgs(void)
{	
    LOG_TRAC("begin");
	for (auto it = wait_read_conns_.begin(); it != wait_read_conns_.end();) {
		ConnPtr conn = *it;
		bool send_empty = conn->send_buf_empty();

		LOG_TRAC("msg_cb_ begin");
		msg_cb_(conn, conn->get_msg_buf());
		LOG_TRAC("msg_cb_ end");
		if (conn->rcv_buf_empty()) {
            LOG_DBUG("The conn is removed from ready_conns: %s", conn->to_str());
			it = wait_read_conns_.erase(it);
		} else {
			++it;
		}
	
		if (conn->is_force_close()) {
			close_conn(conn);
		} else {
			if ((!conn->send_buf_empty() && send_empty) || conn->is_local_fin()) {
				add_conn_wait_write(conn);
			}
		}
	}
    LOG_TRAC("end");
}

void TCPServer::create_signal_fd(void) throw (Errno)
{
    LOG_TRAC("begin");
	if (sig_cb_ || signals_.size()) {
		sigset_t mask;

		if (!(sig_cb_ && signals_.size())) {
			throw Errno("Must specify the signal callback and the signals at the same time", false);
		}

		sigemptyset(&mask);
		for (auto it = signals_.begin(); it != signals_.end(); ++it) {
			if (sigaddset(&mask, *it) == -1) {
				throw Errno("Fail to sigaddset");
			}
			// LOG_DEBUG << "Add signal " << *it << " into sigmask" << endl;
		}

		if (pthread_sigmask(SIG_BLOCK, &mask, NULL) == -1) {
			throw Errno("Fail to pthread_sigmask");
		}

		sig_fd_ = signalfd(-1, &mask, SFD_NONBLOCK|SFD_CLOEXEC);
		if (sig_fd_ == -1) {
			throw Errno("Fail to signalfd");
		}

		if (!epoll_.epoll_add_fd(sig_fd_, EventPoll::EPOLL_EPOLLIN)) {
			throw Errno("Fail to add signal fd");
		}
	}
    LOG_TRAC("end");
}

void TCPServer::process_signals(void) throw (Errno)
{
    LOG_TRAC("begin");
	struct signalfd_siginfo fdsi;
	ssize_t s;

	s = read(sig_fd_, &fdsi, sizeof(fdsi));

	if (s != sizeof(struct signalfd_siginfo))
		throw Errno("Fail to receive singal fd");

	auto it = signals_.find(fdsi.ssi_signo);
	if (it == signals_.end()) {
		throw Errno("Receive unexpected signal", false);
	}

	// LOG_INFO << "Receive signal: " << fdsi.ssi_signo << endl;

	sig_cb_(fdsi.ssi_signo);
    LOG_TRAC("end");
}

void TCPServer::create_timer_fd(void) throw (Errno)
{
    LOG_TRAC("begin");
	if (period_timer_cb_ && period_timer_) {
		period_timer_->start();
		// LOG_INFO << "Period timer is set successfully" << endl;
		
		if (!epoll_.epoll_add_fd(period_timer_->get_fd(), EventPoll::EPOLL_EPOLLIN)) {
			throw Errno("Fail to add timer fd");
		}
		// LOG_INFO << "Period timer starts" << endl;
	}
    LOG_TRAC("end");
}

void TCPServer::process_timer(void) throw (Errno)
{
    LOG_TRAC("begin");
	uint64_t expired_cnt;

	period_timer_->get_expired_cnt(expired_cnt);
	
	period_timer_cb_(expired_cnt, period_timer_data_);
    LOG_TRAC("end");
}

TCPServer::OneshotTimerPtr TCPServer::find_oneshot_timer(int fd) const
{
    LOG_TRAC("begin");
	auto it = oneshot_timers_.find(fd);

	return it == oneshot_timers_.end() ? nullptr: it->second;
    LOG_TRAC("end");
}

void TCPServer::process_oneshot_timer(TCPServer::OneshotTimerPtr &timer)
{
    LOG_TRAC("begin");
	timer->cb_(timer->data_);
	oneshot_timers_.erase(timer->timerfd_->get_fd());
	epoll_.epoll_del_fd(timer->timerfd_->get_fd());
    LOG_TRAC("end");
}

void TCPServer::add_conn_wait_write(const ConnPtr & conn)
{
    LOG_TRAC("begin");
	wait_write_conns_.insert(conn);
	epoll_.epoll_modify_fd(conn->get_fd(), EventPoll::EPOLL_EPOLLIN | EventPoll::EPOLL_EPOLLOUT);
    LOG_TRAC("end");
}

void TCPServer::remove_conn_wait_write(const ConnPtr & conn)
{
    LOG_TRAC("begin");
	wait_write_conns_.erase(conn);

	if (conn->is_remote_fin()) {
		epoll_.epoll_del_fd(conn->get_fd());
		conns_.erase(conn->get_fd());
		conn->close();
	} else {
		epoll_.epoll_modify_fd(conn->get_fd(), EventPoll::EPOLL_EPOLLIN | EventPoll::EPOLL_EPOLLRDHUP);
	}
    LOG_TRAC("end");
}

} // namespace cppbase
