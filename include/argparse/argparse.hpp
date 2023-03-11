//Argument parsing helpers
//
// Parser p(const char *help="", const char *prefix="-", int base=10);
// auto &thing = p.add<type, count=1>(name, help="", default_value={}):
//   return type  arg type  count       description
//   bool         bool  0               : toggle bool flag (-flag -flag -> the default value was)
//   int          bool  1               : count bool flag instances (-flag -flag -> default + 2)
//   vector<T>    T     -1              : vector (variable, multi)
//   T            T     1               : single value of type.  (Passing a default value
//                                        to add() still takes an array, so use braces.)
//   array<T>     T     N>1             : std::array (multi, fixed count)
//   struct       char* Remainder ( -2) : all remaining arguments.  struct
//                                        has argc and argv members.
//
//   missing default value will make the arg required.
//   For non-required multi arguments, premature ending of parsing is
//   allowed.  (The remaining unparsed values just use the defaults).
//
//   The prefix char string should contain 1 character
//
//   An argument is a flag if it begins with the char in prefix.
//   Otherwise, it is a positional argument.  Flags will be arguments
//   given by first specifying the flag and then any values afterwards.
//   Positional arguments are parsed by their position in the argument
//   list.  Multi arg sequences will be interrupted by flags.
//   Flag matching follows these rules:
//     1. First full match.
//     2. If no full matches are found, then partial matches will be
//        checked for flags that were specified with only a single
//        prefix char.  Multiple partial matches will cause parsing
//        to fail.
//
//    If arguments have exactly 2 prefix chars, then they may be handled
//    differently.  If the prefix chars are followed by an int, then it
//    will be parsed.  If it is a positive number N, the following
//    N arguments will be treated as positional arguments.  If there is
//    nothing afterwards, treat all remaining args as positional.
//    Finally, flag names following exactly 2 prefix chars will
//    preferentially match with help.
//
// bool p.parse(int argc, char *argv[])
// bool p.parse(int argc, char *argv[], const char *program_name);
//   If programname is not given, expect it to be argv[0], just like
//   argc and argv passed to main().
//
//   true if parsing succeeded (no error and no help flag):
//   if (!p.parse(...)) { return p.errored; }
//   // do stuff in main()
//
// Parsed values are stored in the returned references.
// The lifetime the references is the same as the parser.
// Parsing uses internal state which may change default values
// of subsequent parses.  Help flags are searched first, and if they
// exist, then no value parsing will occur.
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
	//Struct for iterating over raw arguments.
	struct RawArgs
	{
		char **argv;
		const char *prefix;
		int argc;
		std::size_t isflag;
		int flagskip, base;
		//argc: arg count.
		//argv: argument values.
		//prefix: flag prefix.
		RawArgs(int argc, char *argv[], const char *prefix="-", int base=10):
			argv(argv),
			prefix(prefix),
			argc(argc),
			isflag(argc>0 ? std::strspn(*argv, prefix) : 0),
			flagskip(0),
			base(base)
		{ if (isflag == 2) { calcskip(); } }

		void clear() { argc = 0; }

		void calcskip()
		{
			if (argv[0][2])
			{
				if (!(store(flagskip, argv[0]+isflag, base) && flagskip > 0))
				{ flagskip = 0; }
			}
			else
			{ flagskip = -1; }
			if (flagskip) { isflag = 0; step(); }
		}

		void step()
		{
			if (--argc > 0)
			{
				++argv;
				if (flagskip) { --flagskip; }
				else if ((isflag = std::strspn(*argv, prefix)) == 2) { calcskip(); }
			}
		}
		operator bool() const { return argc > 0; }

		static const int Partial = 1;
		static const int Full = 2;
		//Return whether the current arg prefixes the given name.
		//0 if no match, 1 if prefix, 2 if full match.
		int prefixes(const char *name)
		{
			char *arg = *argv + isflag;
			while (*arg)
			{
				if (*arg != *name) { return 0; }
				++arg, ++name;
			}
			return 1 + (!*name);
		}
		template<class T> bool to(T &dst) { return store(dst, *argv, base); }
	};

	template<> bool RawArgs::to<const char*>(const char* &ptr) { ptr = *argv; return true; }
	template<> bool RawArgs::to<char*>(char* &ptr) { ptr = *argv; return true; }
	template<> bool RawArgs::to<std::string>(std::string &out) { out = *argv; return true; }

	struct Arg
	{
		const char *name;
		const char *help;
		bool required;

		Arg(const char *name, const char *help="", bool required=false):
			name(name), help(help), required(required)
		{}

		void helpshort(std::ostream &o, const char *prefix="")
		{
			const char *wrap = prefix[0] ? "[]" : "<>";
			countsym(o << wrap[0] << prefix << name << " (") << ")" << wrap[1];
		}
		void helplong(std::ostream &o)
		{
			defaults(o);
			if (help[0]) { o << std::endl << "    " << help; }
		}
		virtual std::ostream& countsym(std::ostream &o) = 0;
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
	struct rtype { static std::array<T, N>& get(std::array<T, N> &arr) { return arr; } };
	template<class T>
	struct rtype<T, 1> { static T& get(std::array<T, 1> &arr) { return arr[0]; } };

	//fixed args
	template<class T, int N=1>
	struct TypedArg: public Arg
	{
		static_assert(N>=1, "Default impl requires N >= 1");
		std::array<T,N> value;
		decltype(rtype<T, N>::get(*(std::array<T, N>*)nullptr)) ref()
		{ return rtype<T, N>::get(value); }

		TypedArg(
			const char *name, const char *help="",
			std::array<T,N> value={}, bool required=false
		):
			Arg(name, help, required), value(value)
		{}

		std::ostream& countsym(std::ostream &o) override { return o << 'x' << N; }
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
				if (!(args && !args.isflag && args.to(value[i])))
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
		TypedArg(const char *name, const char *help="", bool value=false, bool required=false):
			Arg(name, help, false), value(value)
		{}

		std::ostream& countsym(std::ostream &o) override { return o << "!!"; }
		void defaults(std::ostream &o) override
		{ o << " (" << value << ")"; }

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

		std::ostream& countsym(std::ostream &o) override { return o << "++"; }
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

		std::ostream& countsym(std::ostream &o) override { return o << (required ? "+" : "*"); }
		void defaults(std::ostream &o) override
		{
			if (!required)
			{
				o << " (";
				for (const auto &v : value) { o << v << ", "; }
				o << ")";
			}
		}

		bool parsevals(RawArgs &args) override
		{
			value.clear();
			T val;
			while (args && !args.isflag && args.to(val))
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

		std::ostream& countsym(std::ostream &o) override { return o << "**"; }
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

	static const char Help[] = "help";
	struct Parser
	{
		std::vector<Arg*> flags;
		std::vector<Arg*> fflags;
		std::vector<Arg*> positionals;
		const char *help;
		const char *prefix;
		std::ostream *logstream;
		std::size_t helplevel;
		int base;
		bool helpmask[4];

		//help: help str for parser
		//prefix: prefix for flags
		//base: base for number parsing.
		Parser(const char *help="", const char *prefix="-", int base=10, std::ostream *logstream=&std::cerr):
			help(help), prefix(prefix), logstream(logstream), helplevel(0), base(base)
		{}

		template<class T, int N=1>
		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(
			const char *name, const char *help, decltype(TypedArg<T, N>::value) value, bool required)
		{
			auto offset = std::strspn(name, prefix);
			if (is_type<T, bool>::value && (N == 0 || N == 1) && !offset)
			{ throw std::logic_error("Single bool args should be flags."); }
			auto *ptr = new TypedArg<T, N>(name+offset, help, value, required);
			if (offset > 1)
			{
				fflags.push_back(ptr);
				const char *check = name+offset;
				int i = 0;
				while (Help[i] && check[i] == Help[i]) { ++i; }
				if (i && !check[i]) { helpmask[i-1] = 1; }
			}
			else if (offset)
			{
				flags.push_back(ptr);
				const char *check = name+offset;
				int i=0;
				while (Help[i] && check[i] == Help[i])
				{ helpmask[i++] = 1; }
			}
			else { positionals.push_back(ptr); }
			return ptr->ref();
		}

		template<class T, int N=1>
		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(const char *name, const char *help="")
		{ return add<T,N>(name, help, {}, true); }
		template<class T, int N=1>
		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(
			const char *name, const char *help, decltype(TypedArg<T, N>::value) value)
		{ return add<T,N>(name, help, value, false); }

		//Search for a matching flag.
		Arg* findflag(RawArgs &args)
		{
			for (Arg *cand: fflags)
			{ if (args.prefixes(cand->name) == args.Full) { return cand; } }
			Arg *best = nullptr;
			int count = 0;
			for (Arg *cand : flags)
			{
				if (int val = args.prefixes(cand->name))
				{
					if (val == args.Full) { return cand; }
					if (!(count++)) { best = cand; }
					else { best = nullptr; }
				}
			}
			if (count > 1 && logstream)
			{ *logstream << "Ambiguous flag: " << args.argv[0] << std::endl; }
			return best;
		}

		void doshort(const char *program, std::ostream &o)
		{
			o << "Usage: " << program;
			for (auto *flag: flags)
			{ flag->helpshort(o << ' ', prefix); }
			for (auto *pos: positionals)
			{ pos->helpshort(o << ' '); }
			o << std::endl;
		}
		void dolong(const char *program, std::ostream &o)
		{
			doshort(program, o);
			if (help[0]) { o << std::endl << help << std::endl; }
			if (flags.size() || fflags.size()) { o << std::endl << "Flags:" << std::endl; }
			for (auto &flaglist : {fflags, flags})
			{
				for (auto *flag: flaglist)
				{
					flag->helpshort(o << "  ", prefix);
					flag->helplong(o);
					o << std::endl;
				}
			}
			if (positionals.size()) { o << std::endl << "Positionals:" << std::endl; }
			for (auto *pos: positionals)
			{
				pos->helpshort(o << "  ");
				pos->helplong(o);
				o << std::endl;
			}
		}
		//return whether help was done.
		bool dohelp(int argc, char *argv[], const char *program)
		{
			RawArgs args(argc, argv, prefix, base);
			helplevel = 0;
			for (; args; args.step())
			{
				if (args.isflag)
				{
					const char *arg = args.argv[0] + args.isflag;
					std::size_t i = 0;
					while (Help[i] && arg[i] == Help[i]) { ++i; }
					if (
						i && !arg[i] && (!helpmask[i-1] || args.isflag == 2)
						&& i > helplevel)
					{ helplevel = i; }
				}
			}
			if (logstream)
			{
				if (helplevel >= 4) { dolong(program, *logstream); }
				else if (helplevel > 0) { doshort(program, *logstream); }
			}
			return helplevel > 0;
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
				if (args.isflag) { if (!(curarg = findflag(args))) { return false; } }
				else if (pit != positionals.end()) { curarg = *(pit++); }
				if (curarg)
				{
					if (args.isflag) { args.step(); }
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
					std::cerr << "unrecognized " << (args.isflag ? "flag" : "positional")
						<< " \"" << args.argv[0] << '"' << std::endl;
					return false;
				}
			}
			while (pit != positionals.end())
			{
				if (!(*pit)->parse(args))
				{
					std::cerr << "Missing arguments for \""
						<< (*pit)->name << '"' << std::endl;
					return false;
				}
				++pit;
			}
			for (auto &flaglist : {fflags, flags})
			{
				for (Arg *flag : flaglist)
				{
					if (flag->required)
					{
						std::cerr << "Missing required flag \"" << prefix << flag->name
							<< '"' << std::endl;
						return false;
					}
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
