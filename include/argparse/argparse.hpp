#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP
#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace argparse
{
	template<class T>
	struct isbool { static const bool value = false; };
	template<>
	struct isbool<bool> { static const bool value = true; };

	struct Parser
	{
		const char *help;
		const char *prefix;
		std::vector<Arg*> pos;
		std::vector<Arg*> flags;

		std::vector<const char*> parsednames;
		decltype(pos.begin()) lastpos;

		Parser(const char *help=nullptr, const char *prefix="-"):
			help(help),
			prefix(prefix),
			pos(),
			flags(),
			parsednames(),
			lastpos()
		{}

		//Add required argument
		template<class T, int count=1>
		TypedArg<T, count> add(const char *name, const char *help)
		{
			return TypedArg<T, count>(
				flagstart(name), help, check<T, count>(name));
		}

		//add optional requirement
		template<class T, int count=1>
		TypedArg<T, count> add(
			const char *name, const char *help,
			std::initializer_list<T> defaults)
		{
			return TypedArg<T, count>(
				flagstart(name), help, check<T, count>(name), defaults);
		}

		//parse main() args
		int parse(int argc, const char *argv[])
		{ return parse(argc-1, argv+1, argv[0]); }

		//Parse arguments only
		int parse(int argc, const char *argv[], const char *program)
		{
			struct ArgIter it(argc, argv, prefix);
			int level = findhelp(it);
			if (level)
			{
				dohelp(program, level);
				return 1;
			}
			lastpos = pos.begin();
			while (it)
			{
				if (it.isflag)
				{
					int idx = findflag(it.flag(), &std::cerr);
					if (idx < 0) { return 2; }
					if (flags[idx]->fill(it))
					{ parsednames.push_back(flags[idx]->name); }
					else
					{
						std::cerr << "Failed to parse argument \"" << it.arg()
							<< "\" for flag " << prefix << flags[idx]->name << std::endl;
						return 2;
					}
				}
				else
				{
					if (lastpos == pos.end())
					{
						std::cerr << "Unexpected positional argument \""
							<< it.arg() << "\"." << std::endl;
						return 2;
					}
					if (!(*lastpos)->fill(it))
					{
						std::cerr << "Error parsing argument " << (*lastpos)->name
							<< '(' << it.arg() << ')' << std::endl;
						return 2;
					}
					++lastpos;
				}
			}
			auto poscheck = lastpos;
			while (poscheck != pos.end())
			{
				if ((*poscheck)->required)
				{
					std::cerr << "Missing required positional argument "
						<< (*poscheck)->name << std::endl;
					return 2;
				}
			}
			//TODO no unparsed required flags.
			return 0;
		}

		//Ensure bool, 0|1 is flag
		//return appropriate vector to store argument.
		template<class T, int count>
		std::vector<Arg*>* check(const char *name)
		{
			bool isflag = name[0] == prefix[0];
			if (isbool<T>::value && (count == 0 || count == 1) && isflag)
			{
				throw std::logic_error(
					std::string(name) + " is <bool, [0|1]> but must be a flag.");
			}
			return isflag ? &flags : &pos;
		}

		const char *flagstart(const char *name)
		{ return name + std::strspn(name, prefix); }

		//-1 exact match
		//0: no match
		//>0: prefix length
		int flagmatch(const char *arg, const char *flag)
		{
			int i = 0;
			while (arg[i] && arg[i] == flag[i]) { ++i; }
			if (arg[i]) { return 0; }
			else if (!flag[i]) { return -1; }
			{ return i; }
		}

		//Find index of best flag.  -1 if no match. -2 if ambiguous
		//best: optional output for the best length
		//  0 for no match
		//  -1 for exact match
		int findflag(const char *name, std::ostream *out=nullptr)
		{
			int pick = -1;
			for (int i=0; i<flags.size(); ++i)
			{
				int length = flagmatch(name, flags[i]->name);
				if (length < 0) { return i; }
				else if (length)
				{
					if (pick == -1) { pick = i; }
					else
					{
						if (pick != -2)
						{
							if (out)
							{
								*out << "Ambiguous flag " << prefix << name << std::endl
									<< "Candidates:" << std::endl
									<< '\t' << prefix << flags[pick]->name << std::endl;
							}
							pick = -2;
						}
						if (out)
						{ *out << '\t' << prefix << flags[i]->name << std::endl; }
					}
				}
			}
			if (pick == -1)
			{
				if (out)
				{
					*out << "Unrecognized flag \""
						<< prefix << name << '"' << std::endl;
				}
			}
			return pick;
		}

		//print help message
		//level:
		//  0: no help message
		//  1: short
		//  2: long
		//
		void dohelp(const char *program, int level)
		{
			if (level < 1) { return; }
			const char *wraps[] = {"[]", "<>"};
			std::cerr << "Usage: " << (program ? program : "program");
			for (Arg *a : flags)
			{
				std::cerr << ' ' << wraps[a->required][0]
					<< prefix << a->flag() << wraps[a->required][1];
			}
			for (Arg *a : pos)
			{
				std::cerr << ' ' << wraps[a->required][0]
					<< a->pos() << wraps[a->required][1];
			}
			std::cerr << std::endl;
			if (level < 2) { return; }
			if (help) { std::cerr << std::endl << help << std::endl; }
			if (flags.size())
			{
				std::cerr << std::endl << "Flags:" << std::endl;
				for (Arg *a : flags)
				{
					std::cerr << wraps[a->required][0] << prefix << a->flag()
						<< wraps[a->required][1];
					a->defaults(std::cerr);
					std::cerr << std::endl;
					if (a->help && a->help[0]) { std::cerr << '\t' <<  a->help << std::endl; }
				}
			}
			if (pos.size())
			{
				std::cerr << std::endl << "Positionals:" << std::endl;
				for (Arg *a : pos)
				{
					std::cerr << wraps[a->required][0] << prefix << a->pos()
						<< wraps[a->required][1];
					a->defaults(std::cerr);
					std::cerr << std::endl;
					if (a->help && a->help[0]) { std::cerr << '\t' <<  a->help << std::endl; }
				}
			}
		}

		//return:
		//  0 no help
		//  1 short help
		//  2 long help
		int findhelp(ArgIter it)
		{
			while (it)
			{
				if (it.isflag)
				{
					int hmatch = flagmatch(it.flag(), "help");
					if (hmatch)
					{
						if (it.isflag == 2) { return (hmatch == -1) + 1; }
						if (findflag(it.flag()) == -1)
						{ return (hmatch == -1) + 1; }
					}
				}
				it.step();
			}
			return 0;
		}
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
