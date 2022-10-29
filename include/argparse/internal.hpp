#ifndef ARGPARSE_INTERNAL_HPP
#define ARGPARSE_INTERNAL_HPP

#include <array>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sstream>
namespace argparse
{
	const char* rawname(const char* name, char prefix);
	bool startswith(const char *value, const char *prefix);
	struct CharCmp
	{
		bool operator()(const char *first, const char *second) const
		{ return std::strcmp(first, second) < 0; }
	};

	struct ArgIter
	{
		char * const *argv;
		int nskips;
		int argc;
		int pos;
		char prefix;
		ArgIter(int argc_, char **argv_, char prefix_='-', int pos_=0):
			argv(argv_), nskips(0), argc(argc_), pos(pos_), prefix(prefix_)
		{}
		const char* operator()() const { return argv[pos]; }
		//next argument
		const char* next();
		const char* peek();
		//next positional argument
		const char* nextpos();
		bool skip(const char *arg);
	};

	struct BaseArg
	{
		const char *name;
		const char *help;
		BaseArg(const char *name_, const char *help_=""):
			name(name_), help(help_)
		{}
		virtual ~BaseArg() = default;
		//argc: number of remaining arguments
		//argv: pointer to first of remaining arguments
		//return: number of args consumed.
		virtual void parse(ArgIter &args) = 0;
		virtual int count() const = 0;
	};

	template<class T, int nargs>
	struct Arg;

	//------------------------------
	// implementation
	//------------------------------
	const char* rawname(const char *name, char prefix)
	{
		while (*name == prefix) { ++name; }
		return name;
	}
	// Get next argument.
	const char* ArgIter::peek()
	{
		if (pos >= argc) { return nullptr; }
		else
		{
			if (!nskips && skip(argv[pos]))
			{ ++pos; return next(); }
			return argv[pos];
		}
	}
	const char* ArgIter::next()
	{
		if (pos >= argc) { return nullptr; }
		else
		{
			if (nskips)
			{ --nskips; }
			else if (skip(argv[pos]))
			{ ++pos; return next(); }
			return argv[pos++];
		}
	}
	// Get next positional argument, or
	// nullpointer if next isn't positional.
	const char* ArgIter::nextpos()
	{
		if (pos >= argc) { return nullptr; }
		if (nskips)
		{ --nskips; }
		else if (argv[pos][0] == prefix)
		{
			if (skip(argv[pos]))
			{ ++pos; return nextpos(); }
			return nullptr;
		}
		return argv[pos++];
	}
	// Return if arg is a skip arg.
	// also set nskips if applicable.
	bool ArgIter::skip(const char *arg)
	{
		const char *name = rawname(arg, prefix);
		if (name-arg == 2)
		{
			if (name[0] == '\0')
			{ nskips = -1; }
			else
			{
				char *end;
				auto val = std::strtoll(name, &end, 10);
				if (*end != '\0' || val < 0)
				{ return false; }
				nskips = val > INT_MAX ? INT_MAX : static_cast<int>(val);
			}
			return true;
		}
		return false;
	}

	//templated conversions
	template<class T>
	T convert_(const char *str, char **end);

	template<>
	long convert_<long>(const char *str, char **end)
	{ return std::strtol(str, end, 10); }
	template<>
	long long convert_<long long>(const char *str, char **end)
	{ return std::strtoll(str, end, 10); }

	template<>
	unsigned long convert_<unsigned long>(const char *str, char **end)
	{
		if (std::strtol(str, end, 10) < 0) { errno = ERANGE; }
		return std::strtoul(str, end, 10);
	}
	template<>
	unsigned long long convert_<unsigned long long>(const char *str, char **end)
	{
		if (std::strtoll(str, end, 10) < 0) { errno = ERANGE; }
		return std::strtoull(str, end, 10);
	}

	template<>
	int convert_<int>(const char *str, char **end)
	{
		long long val = convert_<long long>(str, end);
		if (val < INT_MIN) { errno = ERANGE; }
		else if (val > INT_MAX) { errno = ERANGE; }
		return static_cast<int>(val);
	}
	template<>
	unsigned int convert_<unsigned int>(const char *str, char **end)
	{
		unsigned long long val = convert_<unsigned long long>(str, end);
		if (val > UINT_MAX) { errno = ERANGE; }
		return static_cast<unsigned int>(val);
	}

	template<>
	float convert_<float>(const char *str, char **end)
	{ return std::strtof(str, end); }
	template<>
	double convert_<double>(const char *str, char **end)
	{ return std::strtod(str, end); }

	template<class T>
	T convert(const char *arg)
	{
		char *end;
		errno = 0;
		T ret = convert_<T>(arg, &end);
		if (end == arg || *end != '\0')
		{
			std::string msg("Bad value: \"");
			msg += arg;
			msg += '"';
			throw std::range_error(msg);
		}
		else if (errno == ERANGE)
		{
			std::string msg("value out of range: \"");
			msg += arg;
			msg += '"';
			throw std::range_error(msg);
		}
		return ret;
	}

	template<bool yes, int val>
	struct enable_if {};
	template<int val>
	struct enable_if<true, val>{static const int value=val;};

	//fixed number of args
	template<class T, int nargs>
	struct Arg: public BaseArg
	{
		typedef std::array<T, static_cast<std::size_t>(nargs)> type;
		typedef std::vector<T> def;
		type value;
		bool defaults;

		Arg(const char *name_, const char *help_, def dvals):
			BaseArg(name_, help_),
			value{},
			defaults(dvals.size() > 0)
		{
			if (dvals.size() && dvals.size() != static_cast<std::size_t>(nargs))
			{
				std::stringstream s;
				s << name << " expects " << nargs << " args, but was given "
					<< dvals.size() << " defaults.";
				throw std::logic_error(s.str());
			}
			for (std::size_t i=0; i<dvals.size(); ++i)
			{ value[i] = dvals[i]; }
		}

		virtual int count() const
		{ return enable_if<(nargs>1), nargs>::value; }
		virtual void parse(ArgIter &args)
		{
			const char *arg;
			for (int i=0; i<nargs; ++i)
			{
				if (arg = args.nextpos())
				{ value[i] = convert<T>(arg); }
				else if (!defaults || i>0)
				{
					std::stringstream s;
					s << name << " expects " << nargs << " values, but got "
						<< i << '.';
					throw std::runtime_error(s.str());
				}
				else
				{ return; }
			}
			defaults = true;
		}
	};

	//single arg
	template<class T>
	struct Arg<T, 1>: public BaseArg
	{
		struct def
		{
			T value;
			bool defaults;
			def(): value{}, defaults(0) {}
			def(T val): value{val}, defaults(1) {}
		};
		typedef T type;
		type value;
		bool defaults;
		Arg(const char *name_, const char *help_, def dval):
			BaseArg(name_, help_),
			value{dval.value},
			defaults{dval.defaults}
		{}
		virtual int count() const { return 1; }
		virtual void parse(ArgIter &args)
		{
			const char *arg; 
			if (arg = args.nextpos())
			{ value = convert<T>(arg); }
			else if (!defaults)
			{ throw std::runtime_error(std::string("missing argument ") += name); }
			defaults = true;
		}
	};
	//variable args
	template<class T>
	struct Arg<T, -1>: public BaseArg
	{
		typedef std::vector<T> type;
		typedef type def;
		type value;
		Arg(const char *name_, const char *help_="", type defaults={}):
			BaseArg(name_, help_),
			value{defaults}
		{}
		virtual int count() const { return -1; }
		virtual void parse(ArgIter &args)
		{
			const char *arg;
			if (arg = args.nextpos())
			{
				value.clear();
				value.push_back(convert<T>(arg));
				while (arg = args.nextpos())
				{ value.push_back(convert<T>(arg)); }
			}
		}
	};
	//bool flag
	template<>
	struct Arg<bool, 0>: public BaseArg
	{
		typedef bool type;
		type value;
		Arg(const char *name_, const char *help_="", bool defaults=false):
			BaseArg(name_, help_),
			value{defaults}
		{}
		virtual int count() const { return 0; }
		virtual void parse(ArgIter &args) { value = !value; }
	};

	bool startswith(const char *value, const char *prefix)
	{
		std::size_t i = 0;
		while (value[i] == prefix[i] && value[i] != '\0' && prefix[i] != '\0')
		{ i += 1; }
		return prefix[i] == '\0';
	}
}
#endif
