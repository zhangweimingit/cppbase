#include <errno.h>
#include <memory>
#include "base/utils/compiler.hpp"
#include "base/utils/ik_logger.h"
#include "core/net/conn.hpp"

using namespace std;

namespace cppbase {

class Buffer {
public:
	enum {
		MSG_BUF_MIN_SIZE = 0,
		MSG_BUF_DEFAULT_SIZE = 1024,
	}; 

	Buffer(): data_(MSG_BUF_DEFAULT_SIZE), head_(0), tail_(0) {
	}

	void get_left_space(uint8_t **start, uint32_t *size) 
	{
		*start = &data_[tail_];
		*size = data_.capacity()-tail_; 
	}

	void peek_cur_data(uint8_t **start, uint32_t *size)
	{
		*start = &data_[head_];
		*size = (tail_-head_);
	}
	void append_bytes(uint32_t bytes) 
	{
		tail_ += bytes;
		BUG_ON(tail_ > data_.capacity());
	}
	void consume_bytes(uint32_t bytes) 
	{
		head_ += bytes;
		BUG_ON(head_ > tail_);
	}
	bool full(void) const 
	{
		return ((data_.capacity()-tail_) <= MSG_BUF_MIN_SIZE);
	}
	bool empty(void) const
	{
		return (head_ == tail_);
	}
	void reset(void)
	{
		head_ = tail_ = 0;
	}
	
private:
	vector<uint8_t> data_;
	uint32_t head_;	// The head of data
	uint32_t tail_; // The tail of data
};

void PacketBuf::get_left_space(uint8_t **start, uint32_t *size) 
{		
	BufferPtr buf = bufs_[avail_write_buf_];

	if (buf->full()) {
		if (avail_write_buf_ == bufs_.size()-1) {
			alloc_new_buf();
		}
		avail_write_buf_++;
		buf = bufs_[avail_write_buf_];
	}

	buf->get_left_space(start, size);

    LOG_DBUG("PacketBuf: there are %d bytes space left", *size);
}

void PacketBuf::peek_cur_data(uint8_t **start, uint32_t *size)
{
	BufferPtr buf = bufs_[0];

	buf->peek_cur_data(start, size);
}

void PacketBuf::append_bytes(uint32_t bytes) 
{
	BufferPtr buf = bufs_[avail_write_buf_];

	buf->append_bytes(bytes);
	total_bytes_ += bytes;
}

void PacketBuf::consume_bytes(uint32_t bytes)
{
	BufferPtr buf = bufs_[0];

	buf->consume_bytes(bytes);
	if (buf->empty()) {
		buf->reset();
		if (bufs_.size() > 1) {
			remove_front_buf();
			append_new_buf(buf);
		}
	}
	total_bytes_ -= bytes;
}

void PacketBuf::alloc_new_buf(void)
{		
	BufferPtr buf = std::make_shared<Buffer>();
	append_new_buf(buf);
}


bool Conn::read_bytes(void)
{
	uint8_t *start;
	uint32_t size;
	ssize_t bytes;

	rcv_buf_->get_left_space(&start, &size);

	bytes = recv(fd_, start, size, MSG_DONTWAIT);
	if (-1 == bytes) {
        LOG_WARN("conn(%s) read -1 bytes", to_str());
		return false;
	} else if (0 == bytes) {
        LOG_WARN("conn(%s) closed by peer", to_str());
		return false;
	}
    LOG_DBUG("recv from fd:%d", fd_);
    LOG_DUMP("recv", start, bytes);

	rcv_buf_->append_bytes(bytes);
	return true;
}

void Conn::write_bytes(string &data)
{
	write_bytes(&data[0], data.size());
}

void Conn::write_bytes(void *data, uint32_t data_len)
{
	uint8_t *start;
	uint32_t size;
	uint8_t *write_start;
	uint32_t write_size;
	uint32_t copy_size;
	uint32_t left_size;

	write_start = reinterpret_cast<uint8_t*>(data);
	write_size = 0;

	while (write_size < data_len) {
		send_buf_->get_left_space(&start, &size);
		left_size = data_len-write_size;
		copy_size = min(left_size, size);
		memcpy(start, write_start+write_size, copy_size);
		write_size += copy_size;

		send_buf_->append_bytes(copy_size);
	}
    LOG_DBUG("Conn(%s) writes %d bytes", to_str(), write_size);
}

void Conn::send_bytes(void)
{
	uint8_t *data;
	uint32_t data_len;
	ssize_t bytes;

	do {
		send_buf_->peek_cur_data(&data, &data_len);
		if (data_len == 0) {
			break;
		}
		bytes = send(fd_, data, data_len, MSG_DONTWAIT);
		if (-1 == bytes) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else if (errno == ECONNRESET || errno == EPIPE) {
				// The conn is reset or interrupted by accident
				LOG_ERRO("conn(%s) send failed: %s, force close it",
					to_str(), strerror(errno));
				force_close();
				break;
			} else {
                LOG_WARN("send failed:%s", strerror(errno));
				break;
			}
		}
		send_buf_->consume_bytes(bytes);
        LOG_DBUG("Conn(%s) sends %d bytes", to_str(), bytes);
	} while (1);
}

bool Conn::rcv_buf_empty(void) const
{
	return rcv_buf_->empty();
}

bool Conn::send_buf_empty(void) const
{
	return send_buf_->empty();
}

}

