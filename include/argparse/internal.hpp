#ifndef ARGPARSE_INTERNAL_HPP
#define ARGPARSE_INTERNAL_HPP

#include <array>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace argparse
{
	struct sstr
	{
		std::string str;
		sstr() = default;
		template<class T>
		sstr(T &thing): str{thing} {}
		operator const std::string&() const { return str; }
	};

	const char* rawname(const char* name, char prefix);
	bool startswith(const char *value, const char *prefix);

	struct CharCmp
	{
		bool operator()(const char *first, const char *second) const
		{ return std::strcmp(first, second) < 0; }
	};

	struct ArgIter
	{
		char * const * const argv;
		int nskips;
		const int argc;
		int pos;
		char prefix;
		ArgIter(int argc_, char **argv_, char prefix_='-'):
			argv(argv_), nskips(0), argc(argc_), pos(0), prefix(prefix_)
		{}
		void reset() { pos = 0; nskips = 0; }
		const char* operator()() const { return argv[pos]; }
		//next argument
		const char* next();
		const char* nextflag();
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
		virtual std::string str() const = 0;
		virtual bool ok() const { return true; }
	};

	template<class T, int nargs>
	struct Arg;

	//------------------------------
	// implementation
	//------------------------------
	template<class T>
	sstr& operator<<(sstr &&s, const T &thing)
	{ s << thing; return s; }
	template<class T>
	sstr& operator<<(sstr &s, const T &thing)
	{ s.str += std::to_string(thing); return s; }
	sstr& operator<<(sstr &s, const std::string &thing)
	{ s.str += thing; return s; }
	sstr& operator<<(sstr &s, const char* thing)
	{ s.str += thing; return s; }
	sstr& operator<<(sstr &s, char* thing)
	{ s.str += thing; return s; }
	sstr& operator<<(sstr &s, char thing)
	{ s.str += thing; return s; }

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
	const char* ArgIter::nextflag()
	{
		while (pos < argc)
		{
			if (nskips) { --nskips; }
			else if (argv[pos][0] == prefix && !skip(argv[pos]))
			{ return argv[pos++]; }
			++pos;
		}
		return nullptr;
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

	template<class T>
	struct Argtype { typedef T type; };
	template<>
	struct Argtype<char*> { typedef const char* type; };


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
	typename Argtype<T>::type convert(const char *arg)
	{
		char *end;
		errno = 0;
		T ret = convert_<T>(arg, &end);
		if (end == arg || *end != '\0')
		{ throw std::range_error(sstr("Bad value: \"") << arg << '"'); }
		else if (errno == ERANGE)
		{ throw std::range_error(sstr("value out of range: \"") << arg << '"'); }
		return ret;
	}
	template<>
	const char* convert<const char*>(const char *arg) { return arg; }

	template<bool yes, int val>
	struct enable_if {};
	template<int val>
	struct enable_if<true, val>{static const int value=val;};

	//fixed number of args
	template<class T, int nargs>
	struct Arg: public BaseArg
	{
		typedef typename Argtype<T>::type value_type;
		typedef std::array<value_type, static_cast<std::size_t>(nargs)> type;
		typedef const std::vector<value_type> def;
		type value;
		bool defaults;

		Arg(const char *name_, const char *help_, def dvals):
			BaseArg(name_, help_),
			value{},
			defaults(dvals.size() > 0)
		{
			if (dvals.size() && dvals.size() != static_cast<std::size_t>(nargs))
			{
				throw std::logic_error(
					sstr(name) << " expects " << nargs << " args, but was given "
					<< dvals.size() << " defaults.");
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
				arg = args.nextpos();
				if (arg)
				//if (arg = args.nextpos())
				{ value[i] = convert<value_type>(arg); }
				else
				{
					throw std::runtime_error(
						sstr(name) << " expects " << nargs << " values, but got "
						<< i << '.');
				}
			}
			defaults = true;
		}
		virtual std::string str() const
		{
			if (defaults)
			{
				sstr ret("[");
				auto it = value.begin();
				ret << *(it++);
				while (it != value.end())
				{ ret << ", " << *(it++); }
				ret << ']';
				return ret;
			}
			return "";
		}
		virtual bool ok() const { return defaults; }
	};

	//single arg
	template<class T>
	struct Arg<T, 1>: public BaseArg
	{
		typedef typename Argtype<T>::type value_type;
		struct def
		{
			value_type value;
			bool defaults;
			def(): value{}, defaults(0) {}
			def(value_type val): value{val}, defaults(1) {}
		};
		typedef value_type type;
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
			{ value = convert<value_type>(arg); }
			else
			{ throw std::runtime_error(sstr("missing argument for ") << name); }
			defaults = true;
		}
		virtual std::string str() const
		{
			if (defaults)
			{ return sstr() << value; }
			return "";
		}
		virtual bool ok() const { return defaults; }
	};
	//variable args
	template<class T>
	struct Arg<T, -1>: public BaseArg
	{
		typedef typename Argtype<T>::type value_type;
		typedef std::vector<value_type> type;
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
				value.push_back(convert<value_type>(arg));
				while (arg = args.nextpos())
				{ value.push_back(convert<value_type>(arg)); }
			}
		}
		virtual std::string str() const
		{
			if (value.size())
			{
				sstr s("[");
				auto it = value.begin();
				s << *(it++);
				while (it != value.end())
				{ s << ", " << *(it++); }
				s << ']';
				return s;
			}
			return "";
		}
	};
	//bool flag
	template<>
	struct Arg<bool, 0>: public BaseArg
	{
		typedef bool type, def;
		type value;
		Arg(const char *name_, const char *help_="", bool defaults=false):
			BaseArg(name_, help_),
			value{defaults}
		{}
		virtual int count() const { return 0; }
		virtual void parse(ArgIter &args) { value = !value; }
		virtual std::string str() const
		{ return value ? "true" : "false"; }
	};

	//int flag (count how many occurrences)
	template<>
	struct Arg<int, 0>: public BaseArg
	{
		typedef int type, def;
		type value;
		Arg(const char *name_, const char *help_="", int defaults=0):
			BaseArg(name_, help_),
			value{defaults}
		{}
		virtual int count() const { return 0; }
		virtual void parse(ArgIter &args) { ++value; }
		virtual std::string str() const
		{ return std::to_string(value); }
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
