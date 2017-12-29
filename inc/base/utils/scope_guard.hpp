#ifndef SCOPE_GUARD_H_
#define SCOPE_GUARD_H_

/*
* call some function while leave scope
* usage:
* SCOPE_EXIT {
*     std::cout << "print \n";
* };
*  
*
*/

namespace cppbase {
template <typename FunctionType>
class ScopeGuard {
public:
	explicit ScopeGuard(FunctionType& fn) noexcept
		: dismissed_(false)
		, func_(fn)
	{}

	explicit ScopeGuard(FunctionType& fn, bool dismiss)
		: dismissed_(dismissed_)
	{}

	void dismiss() noexcept
	{
		dismissed_ = true;
	}

	~ScopeGuard()
	{
		if (!dismissed_) {
			func_();
		}
	}

private:
	bool dismissed_;
	FunctionType func_;
};

enum class ScopeGuardOnExit {};

template <typename FunctionType>
ScopeGuard<FunctionType> operator+(ScopeGuardOnExit, FunctionType&& fn) {
	return ScopeGuard<FunctionType>(fn);
}

template <typename FunctionType>
ScopeGuard<FunctionType> makeGuard(FunctionType&& fn) {
	return ScopeGuard<FunctionType>(fn);
}


#define ANONYMOUS_VAR(str) str##__LINE__
#define SCOPE_EXIT \
	auto ANONYMOUS_VAR(SCOPE_EXIT_STATE) \
	= ScopeGuardOnExit() + [&]() noexcept

} //namespace cppbase

#endif

