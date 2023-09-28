#include "argparse/nums.hpp"

namespace argparse
{
#define SPECIALIZE(T, valcheck, prefix) \
	template<> \
	bool store<prefix T>(prefix T &dst, const char *arg, int base) \
	{ \
		prefix long long v; \
		if (store(v, arg, base) && valcheck) \
		{ \
			dst = static_cast<prefix T>(v); \
			return true; \
		} \
		return false; \
	}
	SPECIALIZE(int, INT_MIN <= v && v <= INT_MAX, )
	SPECIALIZE(short, SHRT_MIN <= v && v <= SHRT_MAX, )
	SPECIALIZE(short, v <= USHRT_MAX, unsigned)
	SPECIALIZE(int, v <= UINT_MAX, unsigned)
#undef SPECIALIZE

}
