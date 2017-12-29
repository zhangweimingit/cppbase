#ifndef TIMESTAMP_HPP_
#define TIMESTAMP_HPP_
#include <time.h>

namespace cppbase {
class TimeStamp {
public:
	static unsigned int get_cur_secs(void) {
		return time(NULL);
	}
};
}


#endif

