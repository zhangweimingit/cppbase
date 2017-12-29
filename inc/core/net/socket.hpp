#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <unistd.h>
#include <netinet/in.h>

#include <memory>

namespace cppbase {

struct Socket {
	Socket(): sock_(-1) {};
	~Socket() {
		if (-1 != sock_) {
			close(sock_);
		}
	}

	enum {
		SOCKET_NONBLOCK = 1,
		SOCKET_NONBLOCK_BIT = (1 << SOCKET_NONBLOCK),
		SOCKET_REUSEADDR = 2,
		SOCKET_REUSEADDR_BIT = (1 << SOCKET_REUSEADDR),
		SOCKET_REUSEPORT = 3,
		SOCKET_REUSEPORT_BIT = ( 1 << SOCKET_REUSEPORT),
	};

	enum {
		SOCKET_ANY_ADDR = INADDR_ANY,
	};

	bool open(int domain, int type, int protocol, unsigned int flags = 0);
	bool bind(unsigned int addr, unsigned short port);
	bool bind(const std::string& ip, unsigned short port);
	bool bind_interface(std::string& ifname);
	bool listen(int backlog) {
		return (0 == ::listen(sock_, backlog));
	}
	bool connect(const struct sockaddr *addr, socklen_t addrlen) {
		return (0 == ::connect(sock_, addr, addrlen));
	}

	bool set_timout(uint32_t secs)
	{
		struct timeval timeout;

		timeout.tv_sec = secs;
		timeout.tv_usec = 0;

		if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) {
			return false;
		}
		if (setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
			return false;
		}
		return true;
	}

	bool set_rcv_buf(uint32_t size)
	{
		int opt = size;
		int opt_len = sizeof(opt);

		return (0 == setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, (char*)&opt, opt_len));
	}

	bool set_send_buf(uint32_t size)
	{
		int opt = size;
		int opt_len = sizeof(opt);

		return (0 == setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, (char*)&opt, opt_len));
	}

	int sock_;
};

typedef std::shared_ptr<Socket> SocketPtr;

}



#endif

