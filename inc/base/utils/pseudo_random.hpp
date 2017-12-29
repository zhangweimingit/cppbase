#ifndef PSEUDO_RANDOM_HPP_
#define PSEUDO_RANDOM_HPP_

#include <climits>
#include <random>

namespace cppbase {

class PseudoRandom {
public:
	PseudoRandom() {
		PseudoRandom(INT_MIN, INT_MAX);
	}
	PseudoRandom(int min, int max): gen(rd()), dis(min, max) {
	}
	int generate_random_int(void)
	{
		return dis(gen);
	}
private:
	std::random_device rd;
	std::mt19937 gen;
	std::uniform_int_distribution<> dis;	
};

static inline int get_random_int(int min, int max)
{
	PseudoRandom r(min, max);

	return r.generate_random_int();
}

static inline int get_random_postive_max(int max)
{
	return get_random_int(0, max);
}

static void get_random_string(std::string &s, uint32_t len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"~!@#$%^&*";

	s.clear();
	for (uint32_t i = 0; i < len; ++i) {
		char c = alphanum[get_random_postive_max(sizeof(alphanum)-1)];

		s.push_back(c);
	}
}

}
#endif

