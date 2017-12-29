#ifndef PTHREAD_LOCK_HPP_
#define PTHREAD_LOCK_HPP_

#include <pthread.h>

namespace cppbase {

class Mutex{
public:
	Mutex() {
		pthread_mutex_init(&mutex_, NULL);
	}
	~Mutex() {
		pthread_mutex_destroy(&mutex_);
	}
	void lock() {
		pthread_mutex_lock(&mutex_);
	}
	void unlock() {
		pthread_mutex_unlock(&mutex_);
	}
private:
	pthread_mutex_t mutex_;
};

class RWLock {
public:
	RWLock() {
		pthread_rwlock_init(&lock_, NULL);
	}
	~RWLock() {
		pthread_rwlock_destroy(&lock_);
	}

	void rd_lock() {
		pthread_rwlock_rdlock(&lock_);
	}
	void rd_unlock() {
		pthread_rwlock_unlock(&lock_);
	}
	
	void wr_lock() {
		pthread_rwlock_wrlock(&lock_);
	}
	void wr_unlock() {
		pthread_rwlock_unlock(&lock_);
	}

private:
	pthread_rwlock_t lock_;
};

template <typename T>
class LockGuard {
public:
	LockGuard(T &lock): lock_(lock) {
		lock_.lock();
	}
	~LockGuard() {
		lock_.unlock();
	}
private:
	T &lock_;
};

template <typename T>
class RDLockGuard {
public:
	RDLockGuard(T &lock): lock_(lock) {
		lock_.rd_lock();
	}
	~RDLockGuard() {
		lock_.rd_unlock();
	}
private:
	T &lock_;
};

template <typename T>
class WRLockGuard {
public:
	WRLockGuard(T &lock): lock_(lock) {
		lock_.wr_lock();
	}
	~WRLockGuard() {
		lock_.wr_unlock();
	}
private:
	T &lock_;
};

}

#endif

