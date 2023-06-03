//Argument parsing helpers
//
// usage:
//  Parser p(const char *help="", const char *prefix="-", int base=10);
//  auto arg = p.add<type, count=1>(argname, helpstr, default_values={...});
//  ...
//  p.parse(int argc, const char *argv[]);
//  p.parse(int argc, const char *argv[], const char *program);
//    If program is given then argc, argv are assumed to not contain
//    the program name.  Otherwise, argc and argv are assumed to be as
//    given to main(), argv[0] is the name of the program and arguments
//    start at argv[1].
//    return values:
//      0: success
//      1: help message
//      2: parse error
//    The help flag is searched for first.  If found, then no default
//    values will be changed.
//
// NOTE: Pointers are taken as-is so their lifetime should be at least
// as long as the lifetime of the corresponding args/parser.
//
// The prefix char string should contain 1 character.
// The parsed arguments are directly available as arg.data.
// The returned structs also have methods to access the data member.
//
// The interface of the returned struct depends on count.
//   count    data type                 interface
//     >1:    std::array<type, count>   begin(), end(), operator[](), size()
//     <0:    std::vector<type>         begin(), end(), operator[](), size()
//     1 :    type                      operator type()
//
// Exceptions exist for bool types:
//  count    data type   behavior
//    1:     int         Should be a flag. Count the number of times it is given
//    0:     bool        Should be a flag. Toggle the value of the flag.
//
//  Omitting default values will make the arg required.  Giving default
//  values makes the argument optional (even if empty).
//
//  An argument is a flag if it begins with the char in prefix.  The
//  number of prefix chars does not matter and will all be removed to
//  parse the flag name.  Otherwise, it is a positional argument.
//  Flags will be arguments given by first specifying the flag and then
//  any values afterwards.  Positional arguments are parsed by their
//  position in the argument list.  Multi arg sequences will be
//  interrupted by flags.  Flag matching follows these rules:
//    1. First exact match, in the order the flags were created.
//    2. If no exact matches are found, then the longest match if
//       unique.  If not unique, then the flag is considered ambiguous
//       and parsing will fail.
//
//    NOTE: it is the user's responsibility to ensure no flags have
//    identical names.
//
//  Special flags:
//    --[N]   N: an integer (optional).  The next N arguments will be
//            treated as a positional argument.  If N is omitted, then
//            N will be all remaining arguments.
//    -help   Print help message and stop parsing.  If the flag starts
//            with 2 prefix chars then even if ambiguous, the flag will
//            match with help.  If the flag has the full "help", then
//            the full help message will be given.  Otherwise, a short
//            help message will be given.
//            ex.
//              Parser arguments: "-hello", "-helper"
//              Command line flag:  Match result
//                -hel              Failure, ambiguous (-hello or -helper)
//                --hel[p]          -help (hel is ambiguous, but 2
//                                  prefix chars so prefers help)
//                -help             -helper
//
// help notation:
//   xN: fixed N arguments
//   ...: required multi argument
//   [...]: optional multi argument
//   ++: counting boolean flag
//   !!: toggling boolean flag
// later flags will overwrite newer flags.
#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP
#include "argparse/arg.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace argparse
{
	struct Parser
	{
		const char *help;
		const char *prefix;
		std::vector<Arg*> pos;
		std::vector<Arg*> flags;

		Parser(const char *help="", const char *prefix="-"):
			help(help),
			prefix(prefix),
			pos(),
			flags()
		{}

		template<class T, int count>
		TypedArg<T, count> add(const char *name, const char *help)
		{
			return TypedArg<T, count>(name, help, name[0] == prefix[0] ? &flags : &pos);
		};
	};

//	template<class T, class V>
//	struct is_type { static const bool value = false; };
//	template<class T>
//	struct is_type<T, T> { static const bool value = true; };
//
//	static const char Help[] = "help";
//	struct Parser
//	{
//		std::vector<Arg*> flags;
//		std::vector<Arg*> fflags;
//		std::vector<Arg*> positionals;
//		const char *help;
//		const char *prefix;
//		std::ostream *logstream;
//		std::size_t helplevel;
//		int base;
//		bool helpmask[4];
//
//		//help: help str for parser
//		//prefix: prefix for flags
//		//base: base for number parsing.
//		Parser(const char *help="", const char *prefix="-", int base=10, std::ostream *logstream=&std::cerr):
//			help(help), prefix(prefix), logstream(logstream), helplevel(0), base(base)
//		{}
//
//		template<class T, int N=1>
//		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(
//			const char *name, const char *help, decltype(TypedArg<T, N>::value) value, bool required)
//		{
//			auto offset = std::strspn(name, prefix);
//			if (is_type<T, bool>::value && (N == 0 || N == 1) && !offset)
//			{ throw std::logic_error("Single bool args should be flags."); }
//			auto *ptr = new TypedArg<T, N>(name+offset, help, value, required);
//			if (offset > 1)
//			{
//				fflags.push_back(ptr);
//				const char *check = name+offset;
//				int i = 0;
//				while (Help[i] && check[i] == Help[i]) { ++i; }
//				if (i && !check[i]) { helpmask[i-1] = 1; }
//			}
//			else if (offset)
//			{
//				flags.push_back(ptr);
//				const char *check = name+offset;
//				int i=0;
//				while (Help[i] && check[i] == Help[i])
//				{ helpmask[i++] = 1; }
//			}
//			else { positionals.push_back(ptr); }
//			return ptr->ref();
//		}
//
//		template<class T, int N=1>
//		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(const char *name, const char *help="")
//		{ return add<T,N>(name, help, {}, true); }
//		template<class T, int N=1>
//		decltype(((TypedArg<T, N>*)nullptr)->ref()) add(
//			const char *name, const char *help, decltype(TypedArg<T, N>::value) value)
//		{ return add<T,N>(name, help, value, false); }
//
//		//Search for a matching flag.
//		Arg* findflag(RawArgs &args)
//		{
//			for (Arg *cand: fflags)
//			{ if (args.prefixes(cand->name) == args.Full) { return cand; } }
//			Arg *best = nullptr;
//			int count = 0;
//			for (Arg *cand : flags)
//			{
//				if (int val = args.prefixes(cand->name))
//				{
//					if (val == args.Full) { return cand; }
//					if (!(count++)) { best = cand; }
//					else { best = nullptr; }
//				}
//			}
//			if (count > 1 && logstream)
//			{ *logstream << "Ambiguous flag: " << args.argv[0] << std::endl; }
//			return best;
//		}
//
//		void doshort(const char *program, std::ostream &o)
//		{
//			o << "Usage: " << program;
//			for (auto *flag: flags)
//			{ flag->helpshort(o << ' ', prefix); }
//			for (auto *pos: positionals)
//			{ pos->helpshort(o << ' '); }
//			o << std::endl;
//		}
//		void dolong(const char *program, std::ostream &o)
//		{
//			doshort(program, o);
//			if (help[0]) { o << std::endl << help << std::endl; }
//			if (flags.size() || fflags.size()) { o << std::endl << "Flags:" << std::endl; }
//			for (auto &flaglist : {fflags, flags})
//			{
//				for (auto *flag: flaglist)
//				{
//					flag->helpshort(o << "  ", prefix);
//					flag->helplong(o);
//					o << std::endl;
//				}
//			}
//			if (positionals.size()) { o << std::endl << "Positionals:" << std::endl; }
//			for (auto *pos: positionals)
//			{
//				pos->helpshort(o << "  ");
//				pos->helplong(o);
//				o << std::endl;
//			}
//		}
//		//return whether help was done.
//		bool dohelp(int argc, char *argv[], const char *program)
//		{
//			RawArgs args(argc, argv, prefix, base);
//			helplevel = 0;
//			for (; args; args.step())
//			{
//				if (args.isflag)
//				{
//					const char *arg = args.argv[0] + args.isflag;
//					std::size_t i = 0;
//					while (Help[i] && arg[i] == Help[i]) { ++i; }
//					if (
//						i && !arg[i] && (!helpmask[i-1] || args.isflag == 2)
//						&& i > helplevel)
//					{ helplevel = i; }
//				}
//			}
//			if (logstream)
//			{
//				if (helplevel >= 4) { dolong(program, *logstream); }
//				else if (helplevel > 0) { doshort(program, *logstream); }
//			}
//			return helplevel > 0;
//		}
//
//		// The program name should be argv[0].
//		bool parse(int argc, char *argv[])
//		{
//			char *pname = std::strrchr(argv[0], '/');
//			return parse(argc-1, argv+1, pname ? pname+1 : argv[0]);
//		}
//
//		//Parse without program in argv.
//		bool parse(int argc, char *argv[], const char *program)
//		{
//			//args are mutated from parsing, so must search for help first.
//			if (dohelp(argc, argv, program)) { return false; }
//			RawArgs args(argc, argv, prefix, base);
//			auto pit = positionals.begin();
//			while (args)
//			{
//				Arg *curarg = nullptr;
//				if (args.isflag) { if (!(curarg = findflag(args))) { return false; } }
//				else if (pit != positionals.end()) { curarg = *(pit++); }
//				if (curarg)
//				{
//					if (args.isflag) { args.step(); }
//					if (!curarg->parse(args))
//					{
//						if (args)
//						{
//							std::cerr << "Error parsing token \""
//								<< args.argv[0] << "\" for \"" << curarg->name
//								<< '"' << std::endl;
//						}
//						else
//						{
//							std::cerr << "Missing arguments for \""
//								<< curarg->name << '"' << std::endl;
//						}
//						return false;
//					}
//				}
//				else
//				{
//					std::cerr << "unrecognized " << (args.isflag ? "flag" : "positional")
//						<< " \"" << args.argv[0] << '"' << std::endl;
//					return false;
//				}
//			}
//			while (pit != positionals.end())
//			{
//				if (!(*pit)->parse(args))
//				{
//					std::cerr << "Missing arguments for \""
//						<< (*pit)->name << '"' << std::endl;
//					return false;
//				}
//				++pit;
//			}
//			for (auto &flaglist : {fflags, flags})
//			{
//				for (Arg *flag : flaglist)
//				{
//					if (flag->required)
//					{
//						std::cerr << "Missing required flag \"" << prefix << flag->name
//							<< '"' << std::endl;
//						return false;
//					}
//				}
//			}
//			return true;
//		}
//
//		~Parser()
//		{
//			for (Arg *arg : flags)
//			{ delete arg; }
//			for (Arg *arg : positionals)
//			{ delete arg; }
//		}
//	};
}
#endif // ARGPARSE_HPP
