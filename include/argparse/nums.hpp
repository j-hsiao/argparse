//Parsing numbers from char*
//bool store(T &dst, const char *arg, int base=10)
//
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

	inline void rawcvt(float &dst, const char *args, char **end, int base)
	{ dst = std::strtof(args, end); }
	inline void rawcvt(double &dst, const char *args, char **end, int base)
	{ dst = std::strtod(args, end); }
	inline void rawcvt(long double &dst, const char *args, char **end, int base)
	{ dst =  std::strtold(args, end); }
	inline void rawcvt(long &dst, const char *args, char **end, int base)
	{ dst = std::strtol(args, end, base); }
	inline void rawcvt(long long &dst, const char *args, char **end, int base)
	{ dst = std::strtoll(args, end, base); }
	inline void rawcvt(unsigned long &dst, const char *args, char **end, int base)
	{ dst = std::strtoul(args, end, base); }
	inline void rawcvt(unsigned long long &dst, const char *args, char **end, int base)
	{ dst = std::strtoull(args, end, base); }

	template<class T>
	bool store(T &dst, const char *arg, int base=10)
	{
		char *end = nullptr;
		errno = 0;
		rawcvt(dst, arg, &end, base);
		if (
			(dst && borderval(dst) && errno == ERANGE)
			|| (!dst && end == arg))
		{ return false; }
		//ensure no trailing invalid data.
		//only spaces allowed
		while (std::isspace(*end)) { ++end; }
		return !*end;
	}

#define DECLARE_SPECIALIZATION(T, prefix) \
	template<> \
	bool store<prefix T>(prefix T &dst, const char *arg, int base);

	DECLARE_SPECIALIZATION(short, )
	DECLARE_SPECIALIZATION(int, )
	DECLARE_SPECIALIZATION(short, unsigned)
	DECLARE_SPECIALIZATION(int, unsigned)
#undef DECLARE_SPECIALIZATION

	//A base numbase number.
	template<class T, int numbase>
	struct Base
	{
		static const int base = numbase;
		T data;
		operator T&() { return data; }
		operator const T&() const { return data; }

		T& operator*() { return data; }
		const T& operator*() const { return data; }

		Base& operator=(const T &v) { data = v; return *this; }
	};

	template<class T, int base>
	bool store(Base<T, base> &dst, const char *arg)
	{ return store(dst.data, arg, base); }
}
#endif // ARGPARSE_CONVERT_HPP
