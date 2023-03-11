#ifndef ARGPARSE_CONVERT_HPP
#define ARGPARSE_CONVERT_HPP
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cctype>

namespace argparse
{
	inline bool borderval(float val) { return val == HUGE_VALF; }
	inline bool borderval(double val) { return val == HUGE_VAL; }
	inline bool borderval(long double val) { return val == HUGE_VALL; }
	inline bool borderval(long val) { return val == LONG_MAX || val == LONG_MIN; }
	inline bool borderval(long long val) { return val == LLONG_MAX || val == LLONG_MIN; }
	inline bool borderval(unsigned long val) { return val == ULONG_MAX; }
	inline bool borderval(unsigned long long val) { return val == ULLONG_MAX; }

	void rawcvt(float &dst, const char *args, char **end, int base)
	{ dst = std::strtof(args, end); }
	void rawcvt(double &dst, const char *args, char **end, int base)
	{ dst = std::strtod(args, end); }
	void rawcvt(long double &dst, const char *args, char **end, int base)
	{ dst =  std::strtold(args, end); }
	void rawcvt(long &dst, const char *args, char **end, int base)
	{ dst = std::strtol(args, end, base); }
	void rawcvt(long long &dst, const char *args, char **end, int base)
	{ dst = std::strtoll(args, end, base); }
	void rawcvt(unsigned long &dst, const char *args, char **end, int base)
	{ dst = std::strtoul(args, end, base); }
	void rawcvt(unsigned long long &dst, const char *args, char **end, int base)
	{ dst = std::strtoull(args, end, base); }

	template<class T>
	bool store(T &dst, const char *arg, int base=10)
	{
		char *end = nullptr;
		errno = 0;
		rawcvt(dst, arg, &end, base);
		//example uses this but docs don't explicitly say sets to arg.
		if (end != arg && !(borderval(dst) && errno == ERANGE))
		{
			//ensure no trailing invalid data.
			while (std::isspace(*end)) { ++end; }
			return !*end;
		}
		return false;
	}
#define SPECIALIZE(T, valcheck, prefix) \
	template<> \
	bool store<prefix T>(prefix T &dst, const char *arg, int base) \
	{ \
		prefix long v; \
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
#endif // ARGPARSE_CONVERT_HPP
