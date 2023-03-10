//Argument parsing helpers
//
// Parser p(const char *help="", const char *prefix="-", int base=10);
// auto &thing = p.add<type, count=1>(name, help="", default_value={}):
//   type count
//   bool 0  : toggle bool flag (-flag -flag -> the default value was)
//   bool 1  : count bool flag instances (-flag -flag -> default + 2)
//   T    -1 : vector (variable, multi)
//   T    1  : single value of type.
//   T    N>1: std::array (multi, fixed count)
//
//   missing default value will make the arg required.
//   For non-required multi arguments, premature ending of parsing is
//   allowed.  (The remaining unparsed values just use the defaults).
// bool p.parse(int argc, char *argv[])
// bool p.parse(int argc, char *argv[], const char *program_name);
//   If programname is not given, expect it to be argv[0], just like
//   argc and argv passed to main().
//
//   true if parsing succeeded:
//   if (!p.parse(...)) { return 1; }
//   // do stuff in main()
//
// Parsed values are stored in the returned references.
// The lifetime the references is the same as the parser.
// Parsing uses internal state which may change default values
// of subsequent parses.  Help flags are searched first, and if they
// exist, then no values should change.
//
// help notation:
//   (xN): fixed N arguments
//   (+): required multi argument
//   (*): optional multi argument
//   (**): remainder
//   (++): incrementing boolean flag
//   (!!): toggling boolean flag
// later flags will overwrite newer flags.
#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP
#include "argparse/nums.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace argparse
{
	static const int Remainder = -2;
	struct RawArgs
	{
		char **argv;
		const char *prefix;
		int argc;
		std::size_t offset;
		int flagskip, base;
		//argc: arg count.
		//argv: argument values.
		//prefix: flag prefix.
		RawArgs(int argc, char *argv[], const char *prefix="-", int base=10):
			argv(argv),
			prefix(prefix),
			argc(argc),
			offset(argc>0 ? std::strspn(*argv, prefix) : 0),
			flagskip(0),
			base(base)
		{ if (offset == 2) { calcskip(); } }

		void clear() { argc = 0; }

		void calcskip()
		{
			if (argv[0][2])
			{
				if (store(flagskip, argv[0]+offset, base) && flagskip > 0)
				{ ++flagskip; }
				else { flagskip = 0; }
			}
			else
			{ flagskip = -1; }
			if (flagskip)
			{ offset = 0; step(); }
		}

		RawArgs* step()
		{
			if (--argc > 0)
			{
				++argv;
				if (flagskip)
				{ --flagskip; }
				else
				{
					offset = std::strspn(*argv, prefix);
					if (offset == 2) { calcskip(); }
				}
				return this;
			}
			return nullptr;
		}
		operator bool() const { return argc > 0; }

		//Is current arg a flag?
		bool isflag() { return !flagskip && offset > 0; }

		//Return length of name covered by current arg with caveat.
		//Exact match = -1
		//no match = 0
		int prefixlen(const char *name)
		{
			char *arg = *argv + offset;
			int idx = 0;
			while (arg[idx])
			{
				if (arg[idx] != name[idx])
				{ return 0; }
				++idx;
			}
			return name[idx] ? idx : -1;
		}

		//store to a destination.
		//because lazy so use stringstream
		template<class T>
		bool to(T &dst) { return store(dst, *argv, base); }
	};
	template<>
	bool RawArgs::to<const char*>(const char* &ptr) { ptr = *argv; return true; }
	template<>
	bool RawArgs::to<char*>(char* &ptr) { ptr = *argv; return true; }
	//if arg contains spaces, stringstream would only store up to space.
	template<>
	bool RawArgs::to<std::string>(std::string &out) { out = *argv; return true; }


	struct Arg
	{
		const char *name;
		bool required;
		const char *help;

		Arg(const char *name, const char *help="", bool required=false):
			name(name), help(help), required(required)
		{}

		void helpshort(std::ostream &o, const char *prefix="")
		{
			const char *wrap = prefix[0] ? "[]" : "<>";
			o << wrap[0] << prefix << name << " (";
			countsym(o);
			o << ")" << wrap[1];
		}
		void helplong(std::ostream &o)
		{
			defaults(o);
			if (help[0]) { o << std::endl << '\t' << help; }
		}
		virtual void countsym(std::ostream &o) = 0;
		virtual void defaults(std::ostream &o) {};

		bool parse(RawArgs &args)
		{
			bool ret = parsevals(args);
			required = false;
			return ret;
		}
		virtual bool parsevals(RawArgs &args)=0;
		virtual ~Arg(){}
	};

	template<class T, int N>
	struct rtype {
		typedef std::array<T, N> value_type;
		static value_type& get(std::array<T, N> &arr) { return arr; }
	};
	template<class T>
	struct rtype<T, 1> {
		typedef T value_type;
		static value_type& get(std::array<T, 1> &arr) { return arr[0]; }
	};

	//fixed args
	template<class T, int N=1>
	struct TypedArg: public Arg
	{
		std::array<T,N> value;
		typename rtype<T, N>::value_type& ref()
		{ return rtype<T, N>::get(value); }

		TypedArg(
			const char *name, const char *help="",
			std::array<T,N> value={}, bool required=false
		):
			Arg(name, help, required), value(value)
		{}

		void countsym(std::ostream &o) override { o << 'x' << N; }
		void defaults(std::ostream &o) override
		{
			if (!required)
			{
				o << " (" << value[0];
				for (std::size_t i=1; i<N; ++i)
				{ o << ", " << value[i]; }
				o << ")";
			}
		}

		bool parsevals(RawArgs &args) override
		{
			for (int i=0; i<N; ++i, args.step())
			{
				if (!(args && !args.isflag() && args.to(value[i])))
				{ return !required; }
			}
			return true;
		}
	};
	//invert bool
	template<>
	struct TypedArg<bool, 0>: public Arg
	{
		bool value;
		bool& ref() { return value; }
		TypedArg(const char *name, const char *help="", bool value=true, bool required=false):
			Arg(name, help, false), value(value)
		{}

		void countsym(std::ostream &o) override { o << "!!"; }
		void defaults(std::ostream &o) override
		{ o << " (" << static_cast<int>(value) << ")"; }

		bool parsevals(RawArgs &args) override
		{
			value = !value;
			return true;
		}
	};
	//incrementing bool
	template<>
	struct TypedArg<bool, 1>: public Arg
	{
		int value;
		int& ref() { return value; }
		TypedArg(const char *name, const char *help="", int value=0, bool required=false):
			Arg(name, help, false), value(value)
		{}

		void countsym(std::ostream &o) override { o << "++"; }
		void defaults(std::ostream &o) override { o << "(" << value << ")"; }

		bool parsevals(RawArgs &args) override
		{
			++value;
			return true;
		}
	};

	//var args
	template<class T>
	struct TypedArg<T, -1>: public Arg
	{
		std::vector<T> value;
		std::vector<T>& ref() { return value; }
		TypedArg(const char *name, const char *help="", std::vector<T> value={}, bool required=false):
			Arg(name, help, required),
			value(value)
		{}

		void countsym(std::ostream &o) override { o << (required ? "+" : "*"); }
		void defaults(std::ostream &o) override
		{
			if (value.size())
			{
				o << " (" << value[0];
				for (std::size_t i=1; i<value.size(); ++i)
				{ o << ", " << value[i]; }
				o << ")";
			}
		}

		bool parsevals(RawArgs &args) override
		{
			value.clear();
			T val;
			while (args && !args.isflag() && args.to(val))
			{
				value.push_back(val);
				args.step();
			}
			return true;
		}
	};

	//remainder of args
	template<>
	struct TypedArg<char*, Remainder>: public Arg
	{
		struct Remain
		{
			char **argv;
			int argc;
		};
		Remain value;
		Remain& ref() { return value; }
		TypedArg(const char *name, const char *help="", Remain v={}, bool required=false):
			Arg(name, help, false),
			value(v)
		{}

		void countsym(std::ostream &o) override { o << "**"; }
		bool parsevals(RawArgs &args) override
		{
			value.argv = args.argv;
			value.argc = args.argc;
			args.clear();
			return true;
		}
	};

	template<class T, class V>
	struct is_type { static const bool value = false; };
	template<class T>
	struct is_type<T, T> { static const bool value = true; };

	struct Parser
	{
		std::vector<Arg*> flags;
		std::vector<Arg*> positionals;
		const char *help;
		const char *prefix;
		int base;

		//help: help str for parser
		//prefix: prefix for flags
		//base: base for number parsing.
		Parser(const char *help="", const char *prefix="-", int base=10):
			help(help), prefix(prefix), base(base)
		{}

		template<class T, int N=1>
		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(
			const char *name, const char *help, decltype(TypedArg<T, N>::value) value, bool required)
		{
			if (is_type<T, bool>::value && (N == 0 || N == 1) && name[0] != prefix[0])
			{ throw std::logic_error("Single bool args should be flags."); }
			auto offset = std::strspn(name, prefix);
			auto *ptr = new TypedArg<T, N>(name+offset, help, value, required);
			if (offset)
			{ flags.push_back(ptr); }
			else
			{ positionals.push_back(ptr); }
			return ptr->ref();
		}

		//default value implies not required
		//If required, why default value?
		template<class T, int N=1>
		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(const char *name, const char *help="")
		{ return add<T,N>(name, help, {}, true); }
		template<class T, int N=1>
		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(
			const char *name, const char *help, decltype(TypedArg<T, N>::value) value)
		{ return add<T,N>(name, help, value, false); }

		Arg* findflag(RawArgs &args)
		{
			Arg *best = nullptr;
			int bestlen = 0;
			for (Arg *cand: flags)
			{
				if (int len = args.prefixlen(cand->name))
				{
					if (len < 0)
					{ return cand; }
					else if (len > bestlen)
					{
						best = cand;
						bestlen = len;
					}
				}
			}
			return best;
		}

		void doshort(const char *program)
		{
			std::cerr << "Usage: " << program;
			for (auto *flag: flags)
			{
				std::cerr << ' ';
				flag->helpshort(std::cerr, prefix);
			}
			for (auto *pos: positionals)
			{
				std::cerr << ' ';
				pos->helpshort(std::cerr);
			}
			std::cerr << std::endl;
		}
		void dolong(const char *program)
		{
			doshort(program);
			std::cerr << std::endl;
			if (help[0])
			{ std::cerr << help << std::endl; }
			if (flags.size()) { std::cerr << std::endl << "Flags:" << std::endl; }
			for (auto *flag: flags)
			{
				std::cerr << "  ";
				flag->helpshort(std::cerr, prefix);
				flag->helplong(std::cerr);
				std::cerr << std::endl;
			}
			if (positionals.size()) { std::cerr << std::endl << "Positionals:" << std::endl; }
			for (auto *pos: positionals)
			{
				std::cerr << "  ";
				pos->helpshort(std::cerr);
				pos->helplong(std::cerr);
				std::cerr << std::endl;
			}
		}
		bool dohelp(int argc, char *argv[], const char *program)
		{
			int minscore = 0;
			for (Arg *flag : flags)
			{
				for (int i=0; i<4 && flag->name[i] == ("help")[i] && i >= minscore; ++i)
				{ minscore = i+1; }
			}
			RawArgs args(argc, argv, prefix, base);
			std::size_t level = 0;
			while (args)
			{
				if (args.isflag())
				{
					int score = args.prefixlen("help");
					if ((score < 0 || score > minscore) && args.offset > level)
					{ level = args.offset; }
				}
				args.step();
			}
			if (level >= 2) { dolong(program); }
			else if (level >= 1) { doshort(program); }
			return level > 0;
		}

		// The program name should be argv[0].
		bool parse(int argc, char *argv[])
		{
			char *pname = std::strrchr(argv[0], '/');
			return parse(argc-1, argv+1, pname ? pname+1 : argv[0]);
		}

		//Parse without program in argv.
		bool parse(int argc, char *argv[], const char *program)
		{
			//args are mutated from parsing, so must search for help first.
			if (dohelp(argc, argv, program)) { return false; }
			RawArgs args(argc, argv, prefix, base);
			auto pit = positionals.begin();
			while (args)
			{
				Arg *curarg = nullptr;
				bool isflag = args.isflag();
				if (isflag) { curarg = findflag(args); }
				else if (pit != positionals.end()) { curarg = *(pit++); }
				if (curarg)
				{
					if (isflag) { args.step(); }
					if (!curarg->parse(args))
					{
						if (args)
						{
							std::cerr << "Error parsing token \""
								<< args.argv[0] << "\" for \"" << curarg->name
								<< '"' << std::endl;
						}
						else
						{
							std::cerr << "Missing arguments for \""
								<< curarg->name << '"' << std::endl;
						}
						return false;
					}
				}
				else
				{
					std::cerr << "unrecognized " << (isflag ? "flag" : "positional")
						<< " \"" << args.argv[0] << '"' << std::endl;
					return false;
				}
			}
			while (pit != positionals.end())
			{
				if ((*pit)->required)
				{
					std::cerr << "Missing required positional \""
						<< (*pit)->name << '"' << std::endl;
					return false;
				}
				++pit;
			}
			for (Arg *flag : flags)
			{
				if (flag->required)
				{
					std::cerr << "Missing required flag \"" << prefix << flag->name
						<< '"' << std::endl;
					return false;
				}
			}
			return true;
		}

		~Parser()
		{
			for (Arg *arg : flags)
			{ delete arg; }
			for (Arg *arg : positionals)
			{ delete arg; }
		}
	};
}
#endif // ARGPARSE_HPP
