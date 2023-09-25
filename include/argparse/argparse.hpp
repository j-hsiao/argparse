#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP
#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"

#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include <string>

//#include <array>
//#include <cstring>
//#include <iostream>
//#include <stdexcept>
//#include <string>
//#include <utility>
//#include <vector>

namespace argparse
{
	template<class T>
	const char* most(const std::vector<const char*> &names, T op={})
	{
		auto it = names.begin();
		auto length = std::strlen(*it);
		const char *nm = *it;
		for (; it != names.end(); ++it)
		{
			std::size_t nl = std::strlen(*it);
			if (op(nl, length)) { nm = *it; length = nl; }
		}
		return nm;
	}
	struct lt { bool operator()(std::size_t a, std::size_t b) { return a < b; } };
	struct gt { bool operator()(std::size_t a, std::size_t b) { return a > b; } };

	template<class Parser>
	struct Group
	{
		Parser &parent;
		const char *name;
		std::set<ArgCommon*> members;

		Group(Parser &parent, const char *name):
			parent(parent),
			name(name),
			members{}
		{ parent.groups.push_back(this); }

		void add(ArgCommon &arg)
		{
			parent.add(arg);
			if (!members.insert(&arg).second)
			{ throw std::logic_error("Arg already added: " + std::string(arg.names[0])); }
		}

		void add(FlagCommon &arg)
		{
			parent.add(arg);
			if (!members.insert(&arg).second)
			{
				std::string msg("Flag already added: ");
				const char *name = most<gt>(arg.names);
				msg += parent.prefix;
				if (name[1]) { msg += parent.prefix; }
				msg += name;
				throw std::logic_error(msg);
			}
		}
	};

	struct Parser
	{
		struct Cmp
		{
			bool operator()(const char *a, const char *b) const
			{ return std::strcmp(a, b) < 0; }
		};

		struct ParseResult
		{
			enum codes: int
			{
				success = 0,
				help = 1,
				missing = 2,
				unknown = 3,
				error = 4
			};

			int code;
			std::set<const ArgCommon*> args;
			const Parser *parent;
			operator bool() const { return code; }

			bool parsed(const ArgCommon &arg)
			{ return args.find(&arg) != args.end(); }
		};

		std::vector<ArgCommon*> pos;
		std::vector<Group<Parser>*> groups;
		std::map<const char*, FlagCommon*, Cmp> flags;
		const char *description;
		const char *prefix;
		std::ostream &out;

		Parser(const char *description, const char *prefix, std::ostream &out=std::cerr):
			description(description),
			prefix(prefix),
			out(out)
		{}

		void add(ArgCommon &arg)
		{
			if (arg.names.size() > 1)
			{ throw std::logic_error("Positional arg should have only 1 name."); }
			pos.push_back(&arg);
		}

		void add(FlagCommon &arg)
		{
			for (const char *name : arg.names)
			{
				auto result = flags.insert({name, &arg});
				if (!result.second)
				{ throw std::logic_error("Flag already added."); }
			}
		}

		ParseResult parse(int argc, char *argv[]) const
		{ return parse(argc-1, argv+1, argv[0]); }

		template<class T>
		ParseResult parse(int argc, T *argv, const char *program) const
		{
			ArgIter it(argc, argv, prefix);
			return parse(it, program);
		}

		template<class T, int N>
		ParseResult parse(T (&argv)[N], const char *program) const
		{
			ArgIter it(argv, prefix);
			return parse(it, program);
		}

		ParseResult parse(ArgIter &it, const char *program) const
		{
			if (full_help(it, program)) { return {ParseResult::help, {}, this}; }
			ParseResult result{ParseResult::success, {}, this};
			auto posit = pos.begin();
			while (it)
			{
				if (it.isflag == 1)
				{ if (handle_shortflag(it, result, program)) { return result; } }
				else if (it.isflag)
				{ if (handle_longflag(it, result)) { return result; } }
				else
				{ if (handle_positional(it, result, posit)) { return result; } }
			}
			check_required(result, posit);
			return result;
		}
		private:
			//Search for full help flag. Return true if found or not.
			bool full_help(ArgIter &it, const char *program) const
			{
				while (it)
				{
					if (it.isflag == 2 && !std::strcmp(it.arg, "help"))
					{
						do_fullhelp(program);
						return true;
					}
					it.step();
				}
				it.reset();
				return false;
			}

			void do_shorthelp(const char *program) const
			{
				std::set<const FlagCommon*> handled;
				out << "Usage: " << program;
				const char * wrap[] = {"[]", "<>"};
				for (auto flagpair : flags)
				{
					auto flagpt = flagpair.second;
					if (handled.find(flagpt) != handled.end())
					{ continue; }
					handled.insert(flagpt);
					const char *name = most<lt>(flagpt->names);
					out << ' ' << wrap[flagpt->required][0] << prefix
						<< (name[1] ? prefix : "") << name;
					flagpt->print_count(out) << wrap[flagpt->required][1];
				}

				for (auto argpt : pos)
				{
					out << ' ' << wrap[argpt->required][0] << argpt->names[0];
					argpt->print_count(out) << wrap[argpt->required][1];
				}
				out << std::endl;
			}

			void do_fullhelp(const char *program) const
			{
				do_shorthelp(program);
				if (description)
				{ out << std::endl << description << std::endl; }
				std::set<ArgCommon*> handled;
				for (Group<Parser> *group: groups)
				{
					out << std::endl;
					std::vector<ArgCommon*> groupflags;
					std::vector<ArgCommon*> grouppos;
					for (ArgCommon *arg : group->members)
					{
						auto it = flags.find(arg->names[0]);
						if (it == flags.end() || it->second != arg)
						{ grouppos.push_back(arg); }
						else
						{ groupflags.push_back(arg); }
						handled.insert(arg);
					}
					out << group->name << " args:" << std::endl;
					//TODO
					//print each arg
				}
				//TODO
				//handle leftovers
			}

			int handle_shortflag(ArgIter &it, ParseResult &result, const char *program) const
			{
				char check[2] = {it.arg[0], '\0'};
				auto flag = flags.find(check);
				if (flag == flags.end())
				{
					if (check[0] == 'h' && !it.arg[1])
					{
						do_shorthelp(program);
						return result.code = result.help;
					}
					out << "Unknown flag \"" << prefix << check[0] << '"' << std::endl;
					return result.code = result.unknown;
				}
				it.stepflag();
				if (!flag->second->parse(it))
				{
					out << "Error parsing flag \"" << prefix << check[0] << '"' << std::endl;
					return result.code = result.error;
				}
				result.args.insert(flag->second);
				return 0;
			}

			int handle_longflag(ArgIter &it, ParseResult &result) const
			{
				const char *name = it.arg;
				auto flag = flags.find(name);
				if (flag == flags.end())
				{
					out << "Unknown flag \"" << prefix << prefix << it.arg << '"' << std::endl;
					return result.code = result.unknown;
				}
				it.step();
				if (!flag->second->parse(it))
				{
					out << "Error parsing flag \"" << prefix << prefix << name << '"' << std::endl;
					return result.code = result.error;
				}
				result.args.insert(flag->second);
				return 0;
			}

			int handle_positional(ArgIter &it, ParseResult &result, decltype(pos)::const_iterator &posit) const
			{
				if (posit == pos.end())
				{
					out << "Unknown argument \"" << it.arg << '"' << std::endl;
					return result.code = result.unknown;
				}
				if (!(*posit)->parse(it))
				{
					out << "Error parsing positional \"" << (*posit)->names[0] << '"' << std::endl;
					return result.code = result.error;
				}
				result.args.insert(*posit);
				++posit;
				return 0;
			}

			void check_required(ParseResult &result, decltype(pos)::const_iterator &posit) const
			{
				for (; posit != pos.end(); ++posit)
				{
					if ((*posit)->required)
					{
						out << "Missing required positional argument \"" << (*posit)->names[0] << '"' << std::endl;
						result.code = result.missing;
						return;
					}
				}
				for (auto flagpair : flags)
				{
					if (flagpair.second->required && result.args.find(flagpair.second) == result.args.end())
					{
						const char *name = most<gt>(flagpair.second->names);
						out << "Missing required flag \"" << prefix << (name[1] ? prefix : "") << name << '"' << std::endl;
						result.code = result.missing;
						return;
					}
				}
			}
	};


//	template<class T> struct isbool { static const bool value = false; };
//	template<>
//	struct isbool<bool> { static const bool value = true; };
//
//	struct Parser: public ArgRegistry
//	{
//		struct Arginfo
//		{
//			Arg *arg;
//			const char *groupname;
//			operator Arg*&() { return arg; }
//			Arg& operator*() { return *arg; }
//			Arg* operator->() { return arg; }
//		};
//
//		struct Alias
//		{
//			const char *name;
//			int idx;
//		};
//		const char *help;
//		const char *prefix;
//		std::vector<Arginfo> pos;
//		std::vector<Arginfo> flags;
//		std::vector<Alias> aliases;
//		std::ostream &ostream;
//		Arginfo *lastarg;
//		std::vector<const char *> groups;
//
//		struct ParseResult
//		{
//			Parser *p;
//			std::vector<const char*> parsednames;
//			decltype(p->pos.begin()) lastpos;
//			int code;
//
//			operator bool () const { return code == 0; }
//
//			bool parsed(const char *name) const
//			{
//				if (name[0] == p->prefix[0])
//				{
//					name += std::strspn(name, p->prefix);
//					for (const char *flagname : parsednames)
//					{
//						if (std::strcmp(name, flagname) == 0)
//						{ return true; }
//					}
//					return false;
//				}
//				else
//				{
//					for (auto it = lastpos; it != p->pos.end(); ++it)
//					{
//						if (std::strcmp(name, (*it)->name) == 0)
//						{ return false; }
//					}
//					return true;
//				}
//			}
//		};
//
//		Parser(const char *help=nullptr, const char *prefix="-", std::ostream &out=std::cerr):
//			help(help),
//			prefix(prefix),
//			pos(),
//			flags(),
//			ostream(out),
//			lastarg(nullptr),
//			groups{}
//		{}
//
//		//Add required argument
//		template<class T, int count=1>
//		TypedArg<T, count> add(const char *name, const char *help)
//		{
//			return TypedArg<T, count>(
//				flagstart(name), help, check<T, count>(name));
//		}
//
//		//add optional requirement
//		template<class T, int count=1>
//		TypedArg<T, count> add(
//			const char *name, const char *help,
//			std::initializer_list<typename TypedArg<T, count>::rawtype> defaults)
//		{
//			return TypedArg<T, count>(
//				flagstart(name), help, check<T, count>(name), defaults);
//		}
//
//		template<class T, int count=1>
//		TypedArg<T, count> add(
//			std::initializer_list<const char*> names, const char *help)
//		{
//			add_aliases(names);
//			const char *name = *names.begin();
//			return TypedArg<T, count>(
//				flagstart(name), help, check<T, count>(name));
//		}
//
//		template<class T, int count=1>
//		TypedArg<T, count> add(
//			std::initializer_list<const char*> names, const char *help,
//			std::initializer_list<typename TypedArg<T, count>::rawtype> defaults)
//		{
//			add_aliases(names);
//			const char *name = *names.begin();
//			return TypedArg<T, count>(
//				flagstart(name), help, check<T, count>(name), defaults);
//		}
//
//
//		//parse main() args
//		ParseResult parse(int argc, const char * const argv[])
//		{ return parse(argc-1, argv+1, argv[0]); }
//
//		//Parse arguments only
//		ParseResult parse(int argc, const char * const argv[], const char *program)
//		{
//			ParseResult ret{this, {}, pos.begin(), 0};
//			struct ArgIter it(argc, argv, prefix);
//			int level = findhelp(it);
//			if (level)
//			{
//				dohelp(program, level);
//				ret.code = 1;
//				return ret;
//			}
//			while (it)
//			{
//				if (it.isflag)
//				{
//					if (it.breakpoint())
//					{ it.step(); }
//					else
//					{ parse_flag(it, ret); }
//				}
//				else
//				{ parse_pos(it, ret); }
//				if (ret.code) { return ret; }
//			}
//			auto poscheck = ret.lastpos;
//			while (poscheck != pos.end())
//			{
//				if ((*poscheck)->required)
//				{
//					ostream << "Missing required positional argument "
//						<< (*poscheck)->name << std::endl;
//					ret.code = 2;
//					return ret;
//				}
//				++poscheck;
//			}
//			for (auto &flag : flags)
//			{
//				if (flag->required)
//				{
//					bool wasparsed = false;
//					for (const char *name : ret.parsednames)
//					{
//						if (name == flag->name)
//						{
//							wasparsed = true;
//							break;
//						}
//					}
//					if (!wasparsed)
//					{
//						ostream << "Missing required flag "
//							<< prefix << flag->name << std::endl;
//						ret.code = 2;
//						return ret;
//					}
//				}
//			}
//			return ret;
//		}
//
//		//Ensure bool, 0|1 is flag
//		//return appropriate vector to store argument.
//		template<class T, int count>
//		Parser* check(const char *name)
//		{
//			bool isflag = name[0] == prefix[0];
//			if (isbool<T>::value && (count == 0 || count == 1) && !isflag)
//			{
//				throw std::logic_error(
//					std::string(name) + " is <bool, [0|1]> but must be a flag.");
//			}
//			if (isflag)
//			{
//				flags.push_back({nullptr, nullptr});
//				lastarg = &flags.back();
//			}
//			else
//			{
//				pos.push_back({nullptr, nullptr});
//				lastarg = &pos.back();
//			}
//			return this;
//		}
//
//		const char *flagstart(const char *name)
//		{ return name + std::strspn(name, prefix); }
//
//		//-1 exact match
//		//0: no match
//		//>0: prefix length
//		int flagmatch(const char *arg, const char *flag)
//		{
//			int i = 0;
//			while (arg[i] && arg[i] == flag[i]) { ++i; }
//			if (arg[i]) { return 0; }
//			else if (!flag[i]) { return -1; }
//			{ return i; }
//		}
//
//		//Find index of best flag.  -1 if no match. -2 if ambiguous
//		int findflag(const char *name, std::ostream *out=nullptr)
//		{
//			std::vector<const char *> choices;
//			int pick = -1;
//			for (int i=0; i<flags.size(); ++i)
//			{
//				int length = flagmatch(name, flags[i]->name);
//				if (length < 0) { return i; }
//				else if (length)
//				{
//					if (pick == -1) { pick = i; }
//					else
//					{
//						if (pick != -2)
//						{
//							choices.push_back(flags[pick]->name);
//							pick = -2;
//						}
//						choices.push_back(flags[i]->name);
//					}
//				}
//			}
//			std::vector<Alias*> achoices;
//			for (auto &alias : aliases)
//			{
//				int length = flagmatch(name, alias.name);
//				if (length < 0) { return alias.idx; }
//				else if (length)
//				{
//					achoices.push_back(&alias);
//					if (pick == -1) { pick = alias.idx; }
//					else { pick = -2; }
//				}
//			}
//			if (pick == -1)
//			{
//				if (out)
//				{
//					*out << "Unrecognized flag \""
//						<< prefix << name << '"' << std::endl;
//				}
//			}
//			else if (pick == -2 && out)
//			{
//				*out << "Ambiguous flag \""
//					<< prefix << name << '"' << std::endl
//					<< "Candidates:" << std::endl;
//				for (const char *nm : choices)
//				{ *out << '\t' << prefix << nm << std::endl; }
//				for (const Alias *a : achoices)
//				{
//					*out << '\t' << prefix << a->name << " -> "
//						<< prefix << flags[a->idx]->name << std::endl;
//				}
//			}
//			return pick;
//		}
//
//		//print help message
//		//level:
//		//  0: no help message
//		//  1: short
//		//  2: long
//		//
//		void dohelp(const char *program, int level)
//		{
//			if (level < 1) { return; }
//			const char * const wraps[] = {"[]", "<>"};
//			ostream << "Usage: " << (program ? program : "program");
//			for (auto &a : flags)
//			{
//				ostream << ' ' << wraps[a->required][0]
//					<< prefix << a->flag() << wraps[a->required][1];
//			}
//			for (auto &a : pos)
//			{
//				ostream << ' ' << wraps[a->required][0]
//					<< a->pos() << wraps[a->required][1];
//			}
//			ostream << std::endl;
//			if (level < 2) { return; }
//			longhelp();
//			for (const char *group : groups)
//			{ longhelp(group); }
//		}
//
//		void longhelp(const char *group=nullptr)
//		{
//			const char * const wraps[] = {"[]", "<>"};
//			if (group)
//			{ ostream << std::endl << group << ':' << std::endl; }
//			else if (help)
//			{ ostream << std::endl << help << std::endl << std::endl; }
//			bool header = !group;
//			if (flags.size())
//			{
//				for (auto &a : flags)
//				{
//					if (a.groupname != group) { continue; }
//					if (header)
//					{
//						ostream << std::endl << "Flags:" << std::endl;
//						header = false;
//					}
//					ostream << "  " << wraps[a->required][0] << prefix << a->flag()
//						<< wraps[a->required][1];
//					a->defaults(ostream);
//					ostream << std::endl;
//					auto it = aliases.begin();
//					while (it != aliases.end())
//					{
//						if (flags[it->idx] == a)
//						{
//							ostream << "\tAliases: " << prefix << it->name;
//							++it;
//							while (it != aliases.end() && flags[it->idx] == a)
//							{
//								ostream << ", " << prefix << it->name;
//								++it;
//							}
//							ostream << std::endl;
//							break;
//						}
//						else
//						{ ++it; }
//					}
//					if (a->help && a->help[0]) { ostream << '\t' <<  a->help << std::endl; }
//				}
//			}
//			if (pos.size())
//			{
//				header = !group;
//				for (auto &a : pos)
//				{
//					if (a.groupname != group) { continue; }
//					if (header)
//					{
//						ostream << std::endl << "Positionals:" << std::endl;
//						header = false;
//					}
//					ostream << "  " << wraps[a->required][0] << a->pos()
//						<< wraps[a->required][1];
//					a->defaults(ostream);
//					ostream << std::endl;
//					if (a->help && a->help[0]) { ostream << '\t' <<  a->help << std::endl; }
//				}
//			}
//		}
//
//		//return:
//		//  0 no help
//		//  1 short help
//		//  2 long help
//		int findhelp(ArgIter it)
//		{
//			while (it)
//			{
//				if (it.isflag)
//				{
//					int hmatch = flagmatch(it.flag(), "help");
//					if (hmatch)
//					{
//						if (it.isflag == 2) { return (hmatch == -1) + 1; }
//						if (findflag(it.flag()) == -1)
//						{ return (hmatch == -1) + 1; }
//					}
//				}
//				it.step();
//			}
//			return 0;
//		}
//
//		void parse_pos(ArgIter &it, ParseResult &ret)
//		{
//			if (ret.lastpos == pos.end())
//			{
//				ostream << "Unexpected positional argument \""
//					<< it.arg() << '"' << std::endl;
//				ret.code = 2;
//				return;
//			}
//			int code = (*ret.lastpos)->fill(it);
//			if (code)
//			{
//				if (code == 1)
//				{
//					ostream << "Mising arguments for positional "
//						<< (*ret.lastpos)->name << std::endl;
//				}
//				else if (code == 2)
//				{
//					ostream << "Error parsing argument for positional "
//						<< (*ret.lastpos)->name << ": \"" << it.arg() << '"' << std::endl;
//				}
//				ret.code = 2;
//				return;
//			}
//			++ret.lastpos;
//		}
//
//		void parse_flag(ArgIter &it, ParseResult &ret)
//		{
//			int idx = findflag(it.flag(), &ostream);
//			if (idx < 0)
//			{
//				ret.code = 2;
//				return;
//			}
//			it.step();
//			int code = flags[idx]->fill(it);
//			if (code)
//			{
//				if (code == 1)
//				{
//					ostream << "Missing arguments for flag " << prefix
//						<< flags[idx]->name << std::endl;
//				}
//				else if (code == 2)
//				{
//					ostream << "Failed to parse argument for flag "
//						<< prefix << flags[idx]->name << ": \"" << it.arg() << '"' << std::endl;
//				}
//				ret.code = 2;
//				return;
//			}
//			else
//			{ ret.parsednames.push_back(flags[idx]->name); }
//		}
//
//		void add_aliases(std::initializer_list<const char*> names)
//		{
//			if (!names.size())
//			{ throw std::logic_error("At least 1 name hould be given."); }
//			auto it = names.begin();
//			const char *name = *it;
//			if (name[0] != prefix[0])
//			{ throw std::logic_error("multi-name add requires flags."); }
//			++it;
//			while (it != names.end())
//			{
//				if ((*it)[0] != prefix[0])
//				{ throw std::logic_error("Aliases must be flags."); }
//				aliases.push_back(
//					{flagstart(*it), static_cast<int>(flags.size())});
//				++it;
//			}
//		}
//
//		virtual void push_back(Arg *arg) override
//		{
//			if (!lastarg->arg)
//			{ lastarg->arg = arg; }
//			else
//			{
//				throw std::logic_error(
//					"push_back should only be called once per arg.");
//			}
//		}
//		virtual Arg*& back() override
//		{ return *lastarg; }
//
//		struct Group
//		{
//			Parser *parent;
//			const char *groupname;
//
//			template<class T, int count=1>
//			TypedArg<T, count> add(const char *name, const char *help)
//			{
//				auto x = parent->add<T, count>(name, help);
//				parent->lastarg->groupname = groupname;
//				return x;
//			}
//
//			template<class T, int count=1>
//			TypedArg<T, count> add(
//				const char *name, const char *help,
//				std::initializer_list<typename TypedArg<T, count>::rawtype> defaults)
//			{
//				auto x = parent->add<T, count>(name, help, defaults);
//				parent->lastarg->groupname = groupname;
//				return x;
//			}
//
//			template<class T, int count=1>
//			TypedArg<T, count> add(
//				std::initializer_list<const char*> names, const char *help)
//			{
//				auto x = parent->add<T, count>(names, help);
//				parent->lastarg->groupname = groupname;
//				return x;
//			}
//
//			template<class T, int count=1>
//			TypedArg<T, count> add(
//				std::initializer_list<const char*> names, const char *help,
//				std::initializer_list<typename TypedArg<T, count>::rawtype> defaults)
//			{
//				auto x = parent->add<T, count>(names, help, defaults);
//				parent->lastarg->groupname = groupname;
//				return x;
//			}
//		};
//		
//		Group group(const char *name)
//		{
//			groups.push_back(name);
//			return {this, name};
//		}
//	};
}
#endif // ARGPARSE_HPP
