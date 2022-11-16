// simpler, more basic argument parsing
//
// syntax:
//  flags begin with some prefix (default '-')
//  special flags:
//    --: treat all remaining arguments as positional arguments
//    --N: N is a number, treat the next N arguments as positional
//         arguments.
//    -help: Trigger the help message.  With 1 prefix char, just print
//           the basic usage.  With 2 prefix chars, also add program
//           description if given, and descriptions of each argument.
//           Default values will be shortened if more than 4 values.
//           With 3 or more prefix chars, same as 2 but default value
//           lists are not shortened.
//  Aside from these special flags, the number of prefix chars does
//  not matter.  --myflag and -myflag will be treated the same.
//  If for some reason you want a flag called "1", then --1 will
//  activate the special flags handling.  However, -1 or ---1 will map
//  to a flag called "1"
//  Flags also have uniqueness-matching.  If incomplete and there is exactly
//  1 flag that matches, then that flag will be used.
//  eg. added flags: -myflag -whatever
//      -w will cause -whatever to be used.
//      -m will cause -myflag to be used.
//
// usage:
//    #include "argparse/argparse.hpp"
//
//    argparse::Parser p("description string", prefix='-', program="");
//    type &myref = p.add<type, count>(
//      {name1, name2, ...}|name, help, {default1, default2, ...})
//    if (p.parse(argc-1, argv+1)) { return 1; }
//    if (p.parse_main(argc, argv)) { return 1; }
//    func_that_takes_type_as_arg(myref);
// parse/parse_main will return True if the program should exit.
//  This could be because -help was encountered, or because some error.
//
// If count is 1 or 0, then ref is to type
// If count > 1, then ref is to std::array<type, count>
// If count < 0, then ref is to a std::vector<type>
//
// A count of 0 is applicable for bool and int.
//  If type is bool, then the flag will toggle the value.
//  If type is int, then encountering the flag will increment the value.
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
		Parser(const char *desc=nullptr, char prefix_='-', const char *program_=nullptr):
			program{program_}, description{desc}, prefix{prefix_} {}
		~Parser();

		//flags
		template<class T, int nargs=defaultn<T>::nargs>
		decltype(Arg<T, nargs>::value)& add(
			const std::vector<const char*> &names,
			const char *help="",
			typename Arg<T, nargs>::def default_val={});

		//convenience for flags if only 1 name.
		template<class T, int nargs=defaultn<T>::nargs>
		decltype(Arg<T, nargs>::value)& add(
			const char *name,
			const char *help="",
			typename Arg<T, nargs>::def default_val={});

		//return if parse success
		bool parse(int argc, char *argv[]);
		bool parse_main(int argc, char *argv[])
		{ program = argv[0]; return parse(argc-1, argv+1); }

		const char *program;
		const char *description;
		char prefix;
		private:
			void add_flag(const char *rawname);
			void help(const char *arg="") const;
			std::map<std::size_t, std::vector<const char*>> names() const;
			BaseArg* find_flag(const char *name) const;
			std::vector<BaseArg*> flags;
			std::vector<BaseArg*> positionals;
			std::map<const char*, std::size_t, CharCmp> flagmap;
	};
}
#include "argparse/impl.hpp"
#endif
