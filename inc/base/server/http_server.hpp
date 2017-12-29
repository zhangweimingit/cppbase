#ifndef HTTP_SERVER_HPP_
#define HTTP_SERVER_HPP_
#include "base/server/task_server.hpp"
#include <memory>
#include <functional>
#include <string>

namespace cppbase {

class HTTPRequestImpl;
typedef std::shared_ptr<HTTPRequestImpl> HTTPRequestImplPtr;

class HTTPRequest {
 public:
	typedef std::shared_ptr<HTTPRequest> HTTPRequestPtr;

	HTTPRequest();

	const std::string *get_uri(void);
	const std::string *get_header(const std::string &header_name);

	const std::string *get_header(const char *header_name) {
		std::string tmp = header_name;
		return get_header(tmp);
	}

	const std::string *get_body(void);
 private:
	friend class HTTPServerImpl;
	HTTPRequestImplPtr impl_;
};

class HTTPServerImpl;
typedef std::shared_ptr<HTTPServerImpl> HTTPServerImplPtr;

/*
Param:
	req: The HTTP request
	res: HTTP server would send the res if its length is not zero
Return Value:
	true: Keep the connection
	false: Close the connection
*/
typedef std::function<bool (const HTTPRequest::HTTPRequestPtr &req, std::string &res) > HTTPRequestCallback;
class HTTPServer {
public:
	HTTPServer(uint32_t ip, uint16_t port);
	HTTPServer(std::string ip, uint16_t port)
		: HTTPServer(convert_str_to_ipv4(ip), port) {
	}
	bool init(void);
	void start(void) throw (Errno);
	
	void set_request_callback(const HTTPRequestCallback &cb);
	void set_exit_callback(const ExitCallback &cb);
	void set_signal_callback(const SignalCallback &cb);
	void set_period_timer_callback(const PeriodTimerCallback &cb, void *data);
	void add_oneshot_timer(const OneshotTimerCallback &cb, uint64_t nsecs, void *data) throw (Errno);
	void add_signal(int signo);
	void set_period_timer_interval(uint64_t period_ms);

private:
	HTTPServerImplPtr impl_;
};

typedef std::shared_ptr<HTTPServer> HTTPServerPtr;

} // namespace cppbase

#endif
