#include "argparse/arg.hpp"
#include "argstruct.hpp"
#undef NDEBUG
#include <cassert>
#include <sstream>

int main(int argc, char *argv[])
{
	try
	{
		argparse::TypedArg<int, 3> arg("xyz", "3 values, x, y, z, ints", {1,2});
		assert(false);
	}
	catch (std::exception &) {}

	{
		argparse::TypedArg<int, 3> arg("xyz", "3 values, x, y, z, ints", {1,2,3});
		auto a = args("52", "37", "--1", "-49");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(arg.fill(it));
		assert(arg[0] == 52);
		assert(arg[1] == 37);
		assert(arg[2] == -49);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz x3");
	}
	{
		argparse::TypedArg<int, 3> arg("xyz", "3 values, x, y, z, ints", {1,2,3});
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(!arg.fill(it));
		assert(arg[0] == 52);
		assert(arg[1] == 37);
		assert(it.pos == 3);
	}

	{
		argparse::TypedArg<int, -1> arg("xyz", "3 values, x, y, z, ints", {1,2,3});
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(arg.fill(it));
		assert(arg.size() == 2);
		assert(arg[0] == 52);
		assert(arg[1] == 37);
		assert(it.pos == 3);
		assert(arg.fill(it));
		assert(arg.size() == 0);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz ...");
	}

	{
		argparse::TypedArg<bool> arg("xyz", "3 values, x, y, z, ints");
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(arg.fill(it));
		assert(arg.fill(it));
		assert(arg.fill(it));
		assert(arg == 3);
		assert(arg.fill(it));
		assert(arg.fill(it));
		assert(arg.fill(it));
		assert(arg == 6);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz++");
	}

	{
		argparse::TypedArg<bool, 0> arg("xyz", "3 values, x, y, z, ints");
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(!arg);
		assert(arg.fill(it));
		assert(arg);
		assert(arg.fill(it));
		assert(!arg);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz!!");
	}

	{
		argparse::TypedArg<long, 1> arg("xyz", "3 values, x, y, z, ints");
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(arg.fill(it));
		assert(arg == 52);
		assert(it.pos == 1);
		assert(arg.fill(it));
		assert(arg == 37);
		assert(it.pos == 3);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz");
	}


	return 0;
}
