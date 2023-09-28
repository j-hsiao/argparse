#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP
#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"
#include "argparse/print.hpp"

#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace argparse
{
	struct Group;
	struct Parser;

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

		bool parsed(const ArgCommon &arg) const
		{ return args.find(&arg) != args.end(); }
	};


	struct Parser
	{
		struct Cmp
		{
			bool operator()(const char *a, const char *b) const
			{ return std::strcmp(a, b) < 0; }
		};

		std::vector<ArgCommon*> pos;
		std::vector<Group*> groups;
		std::map<const char*, FlagCommon*, Cmp> flags;
		const char *description;
		const char *prefix;
		std::ostream &out;

		Parser(
			const char *description, const char *prefix="-",
			std::ostream &out=std::cerr);

		void add(ArgCommon &arg);
		void add(FlagCommon &arg);

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

		ParseResult parse(ArgIter &it, const char *program) const;

		private:
			//Search for full help flag. Return true if found or not.
			bool prehelp(ArgIter &it, const char *program) const;

			void do_shorthelp(const char *program) const;

			void do_fullhelp(const char *program) const;

			void flaghelp(const ArgCommon *flag, const char *indent) const;
			void arghelp(
				const ArgCommon *arg, const char *indent,
				const std::map<const char *, int, Cmp> &count) const;

			int handle_shortflag(
				ArgIter &it, ParseResult &result,
				const char *program) const;

			int handle_longflag(ArgIter &it, ParseResult &result) const;

			int handle_positional(
				ArgIter &it, ParseResult &result,
				decltype(pos)::const_iterator &posit) const;

			void check_required(
				ParseResult &result,
				decltype(pos)::const_iterator &posit) const;
	};

	struct Group
	{
		Parser &parent;
		const char *name;
		std::set<ArgCommon*> members;

		Group(Parser &parent, const char *name);
		void add(ArgCommon &arg);
		void add(FlagCommon &arg);
	};
}
#endif // ARGPARSE_HPP
