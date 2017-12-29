// #include "base/utils/logger.hpp"
#include "base/utils/signal.hpp"

using namespace std;

namespace cppbase {

std::map<uint32_t, uint32_t> SignalAction::signal_cnt_;

void SignalAction::signal_action(int sig, siginfo_t * info, void *data)
{	
	// LOG_INFO << "Receive sig: " << info->si_signo << " si_code: " << info->si_code << endl;
	if (info->si_code & SI_USER) {
		// LOG_INFO << "The signal is sent by kill from pid: " << info->si_pid << " uid: " << info->si_uid << endl;
	} else if (info->si_code & SI_KERNEL) {
		// LOG_INFO << "The signal is sent by kernel" << endl;
	}

	auto ret = signal_cnt_.find(sig);
	if (ret != signal_cnt_.end()) {
		signal_cnt_[sig] = signal_cnt_[sig]+1;
	} else {
		signal_cnt_[sig] = 1;
	}
}


bool SignalAction::start_recv_signal(void)
{	
	struct sigaction sigact;
	
	memset(&sigact, 0, sizeof(sigact));
	sigemptyset(&sigact.sa_mask);
	sigact.sa_sigaction = SignalAction::signal_action;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;
	
	for (auto it = sigs_.begin(); it != sigs_.end(); ++it) {
		sigaddset(&sigact.sa_mask, *it);
	}
	
	for (auto it = sigs_.begin(); it != sigs_.end(); ++it) {
		if (-1 == sigaction(*it, &sigact, NULL)) {
			std::cerr << "sigaction failed, %s" << strerror(errno) << std::endl;
			return false;
		}
	}
	
	return true;
}

}

