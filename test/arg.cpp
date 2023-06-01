#include "argparse/arg.hpp"
#include "argstruct.hpp"
#undef NDEBUG
#include <cassert>
#include <cstring>
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
		assert(arg.data[0] == 52);
		assert(arg.data[1] == 37);
		assert(arg.data[2] == -49);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz x3");
	}
	{
		argparse::TypedArg<int, 3> arg("xyz", "3 values, x, y, z, ints", {1,2,3});
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(!arg.fill(it));
		assert(arg.data[0] == 52);
		assert(arg.data[1] == 37);
		assert(!std::strcmp(it.arg(), "notanumber"));
	}

	{
		argparse::TypedArg<int, -1> arg("xyz", "3 values, x, y, z, ints", {1,2,3});
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(arg.fill(it));
		assert(arg.data.size() == 2);
		assert(arg.data[0] == 52);
		assert(arg.data[1] == 37);
		assert(!std::strcmp(it.arg(), "notanumber"));
		assert(arg.fill(it));
		assert(arg.data.size() == 0);

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
		assert(arg.data == 3);
		assert(arg.fill(it));
		assert(arg.fill(it));
		assert(arg.fill(it));
		assert(arg.data == 6);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz ++");
	}

	{
		argparse::TypedArg<bool, 0> arg("xyz", "3 values, x, y, z, ints");
		auto a = args("52", "37", "--1", "notanumber");
		argparse::ArgIter it(a.size(), a.args, "-");
		assert(!arg.data);
		assert(arg.fill(it));
		assert(arg.data);
		assert(arg.fill(it));
		assert(!arg.data);

		std::stringstream s;
		s << arg;
		assert(s.str() == "xyz !!");
	}


	return 0;
}
