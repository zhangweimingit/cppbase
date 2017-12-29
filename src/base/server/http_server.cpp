#include "base/server/http_server_impl.hpp"
#include "base/server/http_server.hpp"
#include "base/utils/ik_logger.h"
#include "base/utils/singleton.hpp"
#include "base/utils/utils.hpp"

#include <locale>
using namespace std;

namespace cppbase {

int on_uri_cb(http_parser * parser, const char * at, size_t length)
{
	HTTPRequestImpl *req = reinterpret_cast<HTTPRequestImpl *> (parser->data);

	req->uri_.append(at, length);
	LOG_DBUG("URI:%s", req->uri_.c_str());

	return 0;
}

int on_header_field_cb(http_parser * parser, const char * at, size_t length)
{
	HTTPRequestImpl *req = reinterpret_cast<HTTPRequestImpl *> (parser->data);

	if (length) {
		req->header_field_.append(at, length);
	}

	return 0;
}

int on_header_value_cb(http_parser * parser, const char * at, size_t length)
{
	HTTPRequestImpl *req = reinterpret_cast<HTTPRequestImpl *> (parser->data);

	if (length) {
		string value(at, length);
		
		StrToLower(req->header_field_);		
		req->headers_[req->header_field_] += value;
		LOG_DBUG("HEADER[%s: %s]", req->header_field_.c_str(), req->headers_[req->header_field_].c_str());
	}

	if (!length || at[length] == '\r') {
		req->header_field_.clear();
	}

	return 0;
}

int on_body_cb(http_parser * parser, const char * at, size_t length)
{
	HTTPRequestImpl *req = reinterpret_cast<HTTPRequestImpl *> (parser->data);
	
	req->body_.append(at, length);

    LOG_DBUG("BODY: %s", req->body_.c_str());
	return 0;
}

int on_headers_complete_cb(http_parser * parser)
{
	return 0;
}

int on_message_complete_cb(http_parser *parser)
{
	HTTPRequestImpl *req = reinterpret_cast<HTTPRequestImpl *> (parser->data);

    LOG_DBUG("keep-alive: %d, parse_state: %d", http_should_keep_alive(parser), parser->state); 

	req->is_completed_ = true;
	return 0;
}

struct HTTPParseSetting {
	HTTPParseSetting() {
		http_parser_settings_init(&setting_);
		setting_.on_url = on_uri_cb;
		setting_.on_header_field = on_header_field_cb;
		setting_.on_header_value = on_header_value_cb;
		setting_.on_body = on_body_cb;
		setting_.on_headers_complete = on_headers_complete_cb;
		setting_.on_message_complete = on_message_complete_cb;
	}
	http_parser_settings setting_;
};


HTTPRequest::HTTPRequest()
{
	impl_ = make_shared<HTTPRequestImpl>();
}

const std::string * HTTPRequest::get_uri(void)
{
	return impl_->get_uri();
}

const std::string * HTTPRequest::get_header(const std::string &header_name)
{
	return impl_->get_http_header(header_name);
}

const std::string * HTTPRequest::get_body(void)
{
	return impl_->get_body();
}

HTTPServer::HTTPServer(uint32_t ip, uint16_t port)
{
	impl_ = make_shared<HTTPServerImpl>(ip, port);
}

bool HTTPServer::init(void)
{
	return impl_->init();
}

void HTTPServer::start(void) throw (Errno)
{
	return impl_->start();
}

void HTTPServer::set_request_callback(const HTTPRequestCallback &cb)
{
	impl_->set_request_callback(cb);
}

void HTTPServer::set_exit_callback(const ExitCallback &cb)
{
	impl_->set_exit_callback(cb);
}

void HTTPServer::set_signal_callback(const SignalCallback &cb)
{
	impl_->set_signal_callback(cb);
}

void HTTPServer::set_period_timer_callback(const PeriodTimerCallback &cb, void *data)
{
	impl_->set_period_timer_callback(cb, data);
}

void HTTPServer::add_oneshot_timer(const OneshotTimerCallback &cb, uint64_t nsecs, void *data) throw (Errno)
{
	impl_->add_oneshot_timer(cb, nsecs, data);
}

void HTTPServer::add_signal(int signo)
{
	impl_->add_signal(signo);
}

void HTTPServer::set_period_timer_interval(uint64_t period_ms)
{
	impl_->set_period_timer_interval(period_ms);
}

bool HTTPServerImpl::init(void)
{
	// set conn callback
	server_.set_conn_callback(bind(&HTTPServerImpl::process_conn, this, std::placeholders::_1, std::placeholders::_2));
	// set msg callback
	server_.set_msg_callback(bind(&HTTPServerImpl::process_msg, this, std::placeholders::_1, std::placeholders::_2));
	
	return server_.init();
}

void HTTPServerImpl::start(void) throw (Errno)
{
	server_.start(NULL);
}

void HTTPServerImpl::process_conn(const ConnPtr & conn, ConnEvent event)
{
    LOG_TRAC();
	if (event == CONN_CONNECTED) {
        LOG_INFO("HTTPServer accepts new conn: %s", conn->to_str());

		HTTPRequest::HTTPRequestPtr req = make_shared<HTTPRequest>();
		conn_reqs_[conn] = req;
	} else {
		LOG_INFO("HTTPServer disconnect conn: %s",  conn->to_str());
		conn_reqs_.erase(conn);
	}
}

void HTTPServerImpl::process_msg(const ConnPtr & conn, PacketBufPtr & msg)
{
    LOG_TRAC("begin");
	auto it = conn_reqs_.find(conn);

	if (it == conn_reqs_.end()) {
		LOG_ERRO("No request for conn: %s", conn->to_str());
        return;
	}

	HTTPRequest::HTTPRequestPtr request = it->second;
	uint8_t *data;
	uint32_t data_len;

	msg->peek_cur_data(&data, &data_len);
	BUG_ON(data_len == 0);

	try {
		if (request->impl_->parse_msg(data, data_len)) {
			if (req_cb_) {
				string response;
				bool keep = req_cb_(request, response);

				if (response.size()) {
					conn->write_bytes(response);
				}

				if (!keep) {
					LOG_DBUG("HTTP Server disconnect the conn: %s", conn->to_str());
					conn->grace_close();
				}
			}
            request->impl_->clear();
		}
        msg->consume_bytes(data_len);
	} catch (string &e) {
		LOG_ERRO("Fail to parse packet: %s", e.c_str());
		return;
	}
    LOG_TRAC("end");
}

HTTPRequestImpl::HTTPRequestImpl()
{
	parser_.data = this;
	
	setting_ = &Singleton<HTTPParseSetting>::instance_ptr()->setting_;
	
	unread_pos_ = 0;
	left_bytes_ = 0;
	clear();
}

bool HTTPRequestImpl::parse_msg(uint8_t * data, uint32_t data_len) throw (std::string)
{
    LOG_TRAC("begin");
    //LOG_DUMP("parse_msg", data, data_len);
	bytes_.insert(bytes_.end(), data, data+data_len);
	left_bytes_ += data_len;

    //LOG_DUMP("http_parser_execute before", &bytes_[unread_pos_], left_bytes_);
    LOG_DBUG("There are %u bytes waiting to parse", left_bytes_);
	size_t parsed_size = http_parser_execute(&parser_, setting_, &bytes_[unread_pos_], left_bytes_);
	LOG_DBUG("http parsed %u bytes", parsed_size);
    //LOG_DUMP("http_parser_execute after", &bytes_[unread_pos_], left_bytes_);
	unread_pos_ += parsed_size;
	left_bytes_ -= parsed_size;

    std::string err_msg = "unknow";
	if (parser_.http_errno) {
        err_msg = http_errno_description(HTTP_PARSER_ERRNO(&parser_));
        LOG_ERRO("invalid HTTP request: %s", err_msg.c_str());
		throw string("Invalid HTTP requst");
	}

    LOG_DBUG("HTTPParser state:%d", parser_.state);

	if (is_completed_ || http_completed_message(&parser_)) {
        LOG_DBUG("receive one completed HTTP request");
		return true;
	}
	LOG_DBUG("Non-completed HTTP request");

    LOG_TRAC("end");
	return false;
}

const string *HTTPRequestImpl::get_uri(void)
{
	if (uri_.size()) {
        LOG_DBUG("uri_:%s", uri_.c_str());
		return &uri_;
	}

	return NULL;
}

const string* HTTPRequestImpl::get_http_header(const string & header_name)
{
	string lower_name = header_name;

	StrToLower(lower_name);

	auto it = headers_.find(lower_name);
	if (it != headers_.end()) {
		return &it->second;
	}

	return NULL;
}

const string *HTTPRequestImpl::get_body(void)
{
	if (body_.size()) {
		return &body_;
	}

	return NULL;
}

void HTTPRequestImpl::clear()
{
	http_parser_init(&parser_, HTTP_REQUEST);
	is_completed_ = false;
	uri_.clear();
	header_field_.clear();
	headers_.clear();
	body_.clear();
}

};
