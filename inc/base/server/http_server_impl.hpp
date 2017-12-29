#ifndef HTTP_SERVER_IMPL_HPP_
#define HTTP_SERVER_IMPL_HPP_

#include <string>
#include <unordered_map>
#include <vector>
#include <map>

#include "base/server/task_server.hpp"
#include "base/server/http_server.hpp"
#include "http-parser/http_parser.h"

namespace cppbase {
class HTTPRequestImpl {
public:
	HTTPRequestImpl();
	/*
	Return Value:
		True: the http request is parsed completely;
		False: The parsing needs more data;
	Exception:
		Meet some errors;
	*/
	bool parse_msg(uint8_t *data, uint32_t data_len) throw(std::string);
	const std::string * get_uri(void);
	const std::string * get_http_header(const std::string &header_name);
	const std::string * get_body(void);
	void clear();

private:
	friend int on_uri_cb(http_parser *parser, const char *at, size_t length);
	friend int on_header_field_cb(http_parser *parser, const char *at, size_t length);
	friend int on_header_value_cb(http_parser *parser, const char *at, size_t length);
	friend int on_body_cb(http_parser *parser, const char *at, size_t length);
	friend int on_headers_complete_cb(http_parser *parser);
	friend int on_message_complete_cb(http_parser *parser);

	std::vector<char> bytes_;
	uint32_t unread_pos_;
	uint32_t left_bytes_;

	http_parser parser_;
	http_parser_settings *setting_;

	std::string uri_;
	std::string header_field_;
	std::unordered_map<std::string, std::string> headers_;
	std::string body_;
	bool is_completed_;
};

class HTTPServerImpl {
public:
	HTTPServerImpl(uint32_t ip, uint16_t port): server_(ip, port) {
	}

	bool init(void);
	void start(void) throw (Errno);

	void set_request_callback(const HTTPRequestCallback &cb)
	{
		req_cb_ = cb;
	}

	void set_exit_callback(const ExitCallback &cb)
	{
		server_.set_exit_callback(cb);
	}

	void set_signal_callback(const SignalCallback &cb)
	{
		server_.set_signal_callback(cb);
	}

	void set_period_timer_callback(const PeriodTimerCallback &cb, void *data)
	{
		server_.set_period_timer_callback(cb, data);
	}

	void add_oneshot_timer(const OneshotTimerCallback &cb, uint64_t nsecs, void *data) throw (Errno)
	{
		server_.add_oneshot_timer(cb, nsecs, data);
	}

	void add_signal(int signo)
	{
		server_.add_signal(signo);
	}
	void set_period_timer_interval(uint64_t period_ms)
	{
		server_.set_period_timer_interval(period_ms);
	}

private:
	void process_conn(const cppbase::ConnPtr &conn, cppbase::ConnEvent event);
	void process_msg(const cppbase::ConnPtr &conn, cppbase::PacketBufPtr &msg);

	TCPServer server_;
	std::map<ConnPtr, HTTPRequest::HTTPRequestPtr> conn_reqs_;

	HTTPRequestCallback req_cb_;
};

}  // namespace cppbase

#endif
