#ifndef SIGNAL_HPP_
#define SIGNAL_HPP_
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <vector>
#include <map>
#include <iostream>

namespace cppbase {

class SignalAction {
public:
	void add_signal(uint32_t sig_nr) 
	{
		sigs_.push_back(sig_nr);
	}
	
	bool start_recv_signal(void);
	
	bool is_recv_signals(void)
	{
		return signal_cnt_.size();
	}

	bool is_recv_signal(uint32_t signal)
	{
		return (signal_cnt_.find(signal) != signal_cnt_.end());
	}

	void reset_signal_cnt(uint32_t signal)
	{
		signal_cnt_.erase(signal);
	}

private:
	static void signal_action(int sig, siginfo_t * info, void *data);
	static std::map<uint32_t, uint32_t> signal_cnt_;
	std::vector<uint32_t> sigs_;
};

static inline void mask_all_signals(void)
{
	sigset_t mask;

	sigemptyset(&mask);
	for (int i = 0; i < SIGRTMAX; ++i) {
		sigaddset(&mask, i);
	}

	pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

}
#endif

