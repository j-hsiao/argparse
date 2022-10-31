// simpler, more basic argument parsing
//
// syntax:
//  flags begin with some prefix (default '-')
//  special flags:
//    --: treat all remaining arguments as positional arguments
//    --N: N is a number, treat the next N arguments as positional
//         arguments.
//  Aside from these special flags, the number of prefix chars does
//  not matter.  --myflag and -myflag will be treated the same.
//  If for some reason you want a flag called "1", then --1 will
//  activate the special flags handling.  However, -1 or ---1 will map
//  to a flag called "1"
//  Flags also have unique-matching.  If incomplete and there is exactly
//  1 flag that matches, then that flag will be used.
//
// usage:
//    argparse::Parser p(prefix='-', program="");
//    type &myref = p.[flag|pos]<type, count>(
//      {name1, name2, ...}|name, help, {default1, default2, ...})
//    p.parse(argc-1, argv+1);
//    p.parse_main(argc, argv);
//    func_that_takes_type_as_arg(placeholder);
//
// if count is 1, then ref is to type
// if count > 1, then ref is to an array of type
// if count < 0, then ref is to a std::vector<type>
// if count == 0, then ref is a bool
//
//
#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <argparse/internal.hpp>

#include <cstddef>
#include <map>
#include <vector>
namespace argparse
{
	struct Parser
	{
		Parser(char prefix_='-', const char *program_=""):
			program(program_), prefix(prefix_) {}
		~Parser();

		//flags
		template<class T, int nargs=1>
		decltype(Arg<T, nargs>::value)& add(
			const std::vector<const char*> &names,
			const char *help="",
			typename Arg<T, nargs>::def default_val={});

		//convenience for flags if only 1 name.
		template<class T, int nargs=1>
		decltype(Arg<T, nargs>::value)& add(
			const char *name,
			const char *help="",
			typename Arg<T, nargs>::def default_val={});

		//return if parse success
		bool parse(int argc, char *argv[]);
		bool parse_main(int argc, char *argv[])
		{ program = argv[0]; return parse(argc-1, argv+1); }

		const char *program;
		char prefix;
		private:
			void add_flag(const char *rawname);
			void help() const;
			std::map<std::size_t, std::vector<const char*>> names() const;
			BaseArg* find_flag(const char *name) const;
			std::vector<BaseArg*> flags;
			std::vector<BaseArg*> positionals;
			std::map<const char*, std::size_t, CharCmp> flagmap;
	};
}
#include "argparse/impl.hpp"
#endif
