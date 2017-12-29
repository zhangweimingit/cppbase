#ifndef CONN_HPP_
#define CONN_HPP_

#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <deque>

#include "base/utils/compiler.hpp"

namespace cppbase {

class Peer {
public:
	enum ProtoFamily {
		PEER_NULL,
		PEER_AF_INET,
		PEER_AF_INET6,
	};

	Peer() {
		proto_family_ = PEER_NULL;
	}

	Peer(ProtoFamily proto_family, const struct sockaddr &addr, socklen_t addrlen) 
	{
		set_peer_info(proto_family, addr, addrlen);
	}

	void set_peer_info(ProtoFamily proto_family, const struct sockaddr &addr, socklen_t addrlen) 
	{
		proto_family_ = proto_family;
		switch (proto_family_) {
			case PEER_AF_INET:
				memcpy(&addr4_, &addr, sizeof(addr4_));
				snprintf(str, sizeof(str), "%u.%u.%u.%u:%u", 
					addr4_.sin_addr.s_addr & 0xFF,
					(addr4_.sin_addr.s_addr>>8) & 0xFF,
					(addr4_.sin_addr.s_addr>>16) & 0xFF,
					addr4_.sin_addr.s_addr>>24, 
					ntohs(addr4_.sin_port)
					);
				break;
			default:
				break;
		}
	}

	const char *to_str(void) const 
	{
		return str;
	}
	
private:
	struct sockaddr_in addr4_;
	ProtoFamily proto_family_;
	char str[48];
};

class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

class PacketBuf {
public:
	PacketBuf(): avail_write_buf_(0), total_bytes_(0) {
		alloc_new_buf();
	}

	void get_left_space(uint8_t **start, uint32_t *size);
	void peek_cur_data(uint8_t **start, uint32_t *size);
	void append_bytes(uint32_t bytes);
	void consume_bytes(uint32_t bytes);

	uint64_t total_size() const
	{
		return total_bytes_;
	}

	bool empty() const 
	{
		return (total_bytes_ == 0);
	}
		
private:
	void append_new_buf(BufferPtr &buf)
	{
		bufs_.push_back(buf);
	}

	void remove_front_buf(void)
	{
		bufs_.pop_front();
		if (avail_write_buf_) {
			avail_write_buf_--;
		}
	}

	void alloc_new_buf(void);

	std::deque<BufferPtr> bufs_;
	uint32_t avail_write_buf_;
	uint64_t total_bytes_;
};
typedef std::shared_ptr<PacketBuf> PacketBufPtr;

class Conn {
public: 
	Conn(int fd): fd_(fd), force_close_(false), local_fin_(false), remote_fin_(false) {
		rcv_buf_ = std::make_shared<PacketBuf>();
		send_buf_ = std::make_shared<PacketBuf>();
	}
	
	~Conn () {
		close();
	}

	void close(void) 
	{
		if (fd_ != -1) {
			// LOG_DEBUG << to_str() << " is closed" << std::endl;
			::close(fd_);
			fd_ = -1;
			local_fin_ = true;
			remote_fin_ = true;
		}
	}

	void grace_close(void) 
	{
		// LOG_DEBUG << to_str() << " grace close" << std::endl;
		set_local_fin();
	}

	bool is_force_close(void) const
	{
		return force_close_;
	}
	void force_close(void)
	{
		// LOG_DEBUG << to_str() << " force close" << std::endl;
		force_close_ = true;
	}

	bool is_local_fin(void) const
	{
		return local_fin_;
	}
	void set_local_fin(void)
	{
		local_fin_ = true;
	}

	bool is_remote_fin(void) const
	{
		return remote_fin_;
	}
	void set_remote_fin(void)
	{
		// LOG_DEBUG << to_str() << " remote close" << std::endl;
		remote_fin_ = true;
	}

	bool read_bytes(void);
	void write_bytes(std::string &data);
	void write_bytes(void *data, uint32_t data_len);

	void set_peer_info(Peer::ProtoFamily proto_family, const struct sockaddr &addr, socklen_t addrlen) 
	{
		peer_.set_peer_info(proto_family, addr, addrlen);
	}

	const char * to_str(void) const
	{
		return peer_.to_str();
	}

	int get_fd(void) const
	{
		return fd_;
	}

	PacketBufPtr & get_msg_buf(void)
	{
		return rcv_buf_;
	}

	void send_bytes(void);
	
	bool rcv_buf_empty(void) const;
	bool send_buf_empty(void) const;
	
private:
	PacketBufPtr rcv_buf_;
	PacketBufPtr send_buf_;
	Peer peer_;
	int fd_;

	bool force_close_;
	bool local_fin_;
	bool remote_fin_;
};

typedef std::shared_ptr<Conn> ConnPtr;

}

#endif

