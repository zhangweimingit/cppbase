#ifndef ERRNO_HPP_
#define ERRNO_HPP_

#include <exception>
#include <string>
#include <sstream>

#include "string.h"
#include "errno.h"

namespace cppbase {

class Errno: public std::exception {
public:
	Errno(bool append_errstr = true) {
		generate_errstr("", append_errstr);
	}

	Errno(const char* err, bool append_errstr = true) {
		generate_errstr(err, append_errstr);
	}
	
	Errno(const std::string &err, bool append_errstr = true) {
		generate_errstr(err, append_errstr);
	}

	const char* what(void) const noexcept {
		return err_.c_str();
	}
	
private:
	void generate_errstr(const std::string &err, bool append_errstr) {
		std::stringstream ss;
		ss << err;

		if (append_errstr) {
			if (err.size()) {
				ss << ": ";
			}
			ss << strerror(errno) << "(" << errno << ")";
		}

		err_ = ss.str();
	}

	std::string err_;
};

}

#endif
