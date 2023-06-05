#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP
#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace argparse
{
	template<class T>
	struct isbool { static const bool value = false; };
	template<>
	struct isbool<bool> { static const bool value = true; };

	struct Parser
	{
		struct Alias
		{
			const char *name;
			int idx;
		};
		const char *help;
		const char *prefix;
		std::vector<Arg*> pos;
		std::vector<Arg*> flags;
		std::vector<Alias> aliases;
		std::ostream &ostream;

		struct ParseResult
		{
			Parser *p;
			std::vector<const char*> parsednames;
			decltype(p->pos.begin()) lastpos;
			int code;

			operator bool () const { return code == 0; }

			bool parsed(const char *name) const
			{
				if (name[0] == p->prefix[0])
				{
					name += std::strspn(name, p->prefix);
					for (const char *flagname : parsednames)
					{
						if (std::strcmp(name, flagname) == 0)
						{ return true; }
					}
					return false;
				}
				else
				{
					for (auto it = lastpos; it != p->pos.end(); ++it)
					{
						if (std::strcmp(name, (*it)->name) == 0)
						{ return false; }
					}
					return true;
				}
			}
		};

		Parser(const char *help=nullptr, const char *prefix="-", std::ostream &out=std::cerr):
			help(help),
			prefix(prefix),
			pos(),
			flags(),
			ostream(out)
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

		template<class T, int count=1>
		TypedArg<T, count> add(
			std::initializer_list<const char*> names, const char *help)
		{
			add_aliases(names);
			const char *name = *names.begin();
			return TypedArg<T, count>(
				flagstart(name), help, check<T, count>(name));
		}

		template<class T, int count=1>
		TypedArg<T, count> add(
			std::initializer_list<const char*> names, const char *help,
			std::initializer_list<T> defaults)
		{
			add_aliases(names);
			const char *name = *names.begin();
			return TypedArg<T, count>(
				flagstart(name), help, check<T, count>(name), defaults);
		}


		//parse main() args
		ParseResult parse(int argc, const char * const argv[])
		{ return parse(argc-1, argv+1, argv[0]); }

		//Parse arguments only
		ParseResult parse(int argc, const char * const argv[], const char *program)
		{
			ParseResult ret{this, {}, pos.begin(), 0};
			struct ArgIter it(argc, argv, prefix);
			int level = findhelp(it);
			if (level)
			{
				dohelp(program, level);
				ret.code = 1;
				return ret;
			}
			while (it)
			{
				if (it.isflag)
				{ parse_flag(it, ret); }
				else
				{ parse_pos(it, ret); }
				if (ret.code) { return ret; }
			}
			auto poscheck = ret.lastpos;
			while (poscheck != pos.end())
			{
				if ((*poscheck)->required)
				{
					ostream << "Missing required positional argument "
						<< (*poscheck)->name << std::endl;
					ret.code = 2;
					return ret;
				}
				++poscheck;
			}
			for (Arg *flag : flags)
			{
				if (flag->required)
				{
					bool wasparsed = false;
					for (const char *name : ret.parsednames)
					{
						if (name == flag->name)
						{
							wasparsed = true;
							break;
						}
					}
					if (!wasparsed)
					{
						ostream << "Missing required flag "
							<< prefix << flag->name << std::endl;
						ret.code = 2;
						return ret;
					}
				}
			}
			return ret;
		}

		//Ensure bool, 0|1 is flag
		//return appropriate vector to store argument.
		template<class T, int count>
		std::vector<Arg*>* check(const char *name)
		{
			bool isflag = name[0] == prefix[0];
			if (isbool<T>::value && (count == 0 || count == 1) && !isflag)
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
		int findflag(const char *name, std::ostream *out=nullptr)
		{
			std::vector<const char *> choices;
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
							choices.push_back(flags[pick]->name);
							pick = -2;
						}
						choices.push_back(flags[i]->name);
					}
				}
			}
			std::vector<Alias*> achoices;
			for (auto &alias : aliases)
			{
				int length = flagmatch(name, alias.name);
				if (length < 0) { return alias.idx; }
				else if (length)
				{
					achoices.push_back(&alias);
					if (pick == -1) { pick = alias.idx; }
					else { pick = -2; }
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
			else if (pick == -2 && out)
			{
				*out << "Ambiguous flag \""
					<< prefix << name << '"' << std::endl
					<< "Candidates:" << std::endl;
				for (const char *nm : choices)
				{ *out << '\t' << prefix << nm << std::endl; }
				for (const Alias *a : achoices)
				{
					*out << '\t' << prefix << a->name << " -> "
						<< prefix << flags[a->idx]->name << std::endl;
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
			ostream << "Usage: " << (program ? program : "program");
			for (Arg *a : flags)
			{
				ostream << ' ' << wraps[a->required][0]
					<< prefix << a->flag() << wraps[a->required][1];
			}
			for (Arg *a : pos)
			{
				ostream << ' ' << wraps[a->required][0]
					<< a->pos() << wraps[a->required][1];
			}
			ostream << std::endl;
			if (level < 2) { return; }
			if (help) { ostream << std::endl << help << std::endl; }
			if (flags.size())
			{
				ostream << std::endl << "Flags:" << std::endl;
				for (Arg *a : flags)
				{
					ostream << wraps[a->required][0] << prefix << a->flag()
						<< wraps[a->required][1];
					a->defaults(ostream);
					ostream << std::endl;
					auto it = aliases.begin();
					while (it != aliases.end())
					{
						if (flags[it->idx] == a)
						{
							ostream << "\tAliases: " << prefix << it->name;
							++it;
							while (it != aliases.end() && flags[it->idx] == a)
							{
								ostream << ", " << prefix << it->name;
								++it;
							}
							ostream << std::endl;
							break;
						}
						else
						{ ++it; }
					}
					if (a->help && a->help[0]) { ostream << '\t' <<  a->help << std::endl; }
				}
			}
			if (pos.size())
			{
				ostream << std::endl << "Positionals:" << std::endl;
				for (Arg *a : pos)
				{
					ostream << wraps[a->required][0] << a->pos()
						<< wraps[a->required][1];
					a->defaults(ostream);
					ostream << std::endl;
					if (a->help && a->help[0]) { ostream << '\t' <<  a->help << std::endl; }
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

		void parse_pos(ArgIter &it, ParseResult &ret)
		{
			if (ret.lastpos == pos.end())
			{
				ostream << "Unexpected positional argument \""
					<< it.arg() << '"' << std::endl;
				ret.code = 2;
				return;
			}
			int code = (*ret.lastpos)->fill(it);
			if (code)
			{
				if (code == 1)
				{
					ostream << "Mising arguments for positional "
						<< (*ret.lastpos)->name << std::endl;
				}
				else if (code == 2)
				{
					ostream << "Error parsing argument for positional "
						<< (*ret.lastpos)->name << ": \"" << it.arg() << '"' << std::endl;
				}
				ret.code = 2;
				return;
			}
			++ret.lastpos;
		}

		void parse_flag(ArgIter &it, ParseResult &ret)
		{
			int idx = findflag(it.flag(), &ostream);
			if (idx < 0)
			{
				ret.code = 2;
				return;
			}
			it.step();
			int code = flags[idx]->fill(it);
			if (code)
			{
				if (code == 1)
				{
					ostream << "Missing arguments for flag " << prefix
						<< flags[idx]->name << std::endl;
				}
				else if (code == 2)
				{
					ostream << "Failed to parse argument for flag "
						<< prefix << flags[idx]->name << ": \"" << it.arg() << '"' << std::endl;
				}
				ret.code = 2;
				return;
			}
			else
			{ ret.parsednames.push_back(flags[idx]->name); }
		}

		void add_aliases(std::initializer_list<const char*> names)
		{
			if (!names.size())
			{ throw std::logic_error("At least 1 name hould be given."); }
			auto it = names.begin();
			const char *name = *it;
			if (name[0] != prefix[0])
			{ throw std::logic_error("multi-name add requires flags."); }
			++it;
			while (it != names.end())
			{
				if ((*it)[0] != prefix[0])
				{ throw std::logic_error("Aliases must be flags."); }
				aliases.push_back(
					{flagstart(*it), static_cast<int>(flags.size())});
				++it;
			}
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
