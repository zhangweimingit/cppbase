#ifndef FGAO_THREAD_HPP_
#define FGAO_THREAD_HPP_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "base/utils/noncopyable.hpp"
#include "core/thread/thread_impl.hpp"

namespace cppbase {

class Thread: noncopyable {
public:
	typedef std::function<void (void*)> ThreadFunc;

	struct ThreadData {
		ThreadData(ThreadFunc func, void *data, std::string name, bool mask_signals): func_(func), data_(data),
			name_(name), mask_signals_(mask_signals) {
		};
		ThreadFunc func_;
		void *data_;
		std::string name_;
		bool mask_signals_;
	};
	
	Thread(const ThreadFunc &func, void *data, const std::string &name = std::string(), bool detach = true, bool mask_signal = false);
	Thread(const std::string &name = std::string(), bool detach = true, bool mask_signal = false);
	
	void start(void);
	void wait_stoped(void);
	bool set_cpu_affinity(size_t cpusetsize, const cpu_set_t *cpuset);
	
	void set_thread_func(const ThreadFunc func)
	{
		func_ = func;
	}

	void set_thread_data(void *data)
	{
		data_ = data;
	}

private:	
	static void* start_thread(void*);

	ThreadFunc func_;
	void *data_;	
	std::string name_;
	bool detach_;
	bool mask_signals_;
	pthread_t thread_id_;
};

typedef std::shared_ptr<Thread>		ThreadPtr;

class ThreadPool {
public:
	ThreadPool(bool set_cpu = true, uint32_t start_cpu = 0, uint32_t max_cpu = -1u);
	void append_thread(ThreadPtr thr) 
	{
		threads_.push_back(thr);
	}
	void start_all_threads(void);
	void wait_all_threads_stoped(void);
private:
	std::vector<ThreadPtr> threads_;
	bool set_cpu_;
	uint32_t cur_cpu_;
	uint32_t start_cpu_;
	uint32_t max_cpu_;
};


}


#endif

