#ifndef COMPILER_HPP_
#define COMPILER_HPP_

// #include "logger.hpp"

namespace cppbase {
#ifndef likely
#define likely(x)	__builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

#define CRASH()		(* reinterpret_cast<uint32_t*>(NULL) == 0)
/*
#define BUG() \
	LOG_ALERT << "BUG: Unexpected failure at " << __FILE__ << ":" << __LINE__ << "/" << __func__ << std::endl; \
	CRASH();
*/
#define BUG() \
	CRASH();

#define BUG_ON(x) \
	if (unlikely(x)) { \
		BUG(); \
	}

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

}
#endif

