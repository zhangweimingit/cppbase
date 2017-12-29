#ifndef CPPBASE_UTILS_NON_COPYABLE_HPP_
#define CPPBASE_UTILS_NON_COPYABLE_HPP_

namespace cppbase {
	class noncopyable {
	protected:
		noncopyable() {}
		~noncopyable() {}

	private:
		noncopyable(const noncopyable&);	
		noncopyable& operator=(const noncopyable&);
	};
}

#endif

