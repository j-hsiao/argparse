#undef NDEBUG

#include "argparse/argparse.hpp"
#include "argparse/argiter.hpp"
#include "argstruct.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>

int basics(const char *prog)
{
	std::stringstream ss;
	using namespace argparse;
	Parser p("Test program", "-", ss);
	Arg<int> num(p, "num", "A number, required");
	assert(num.required);

	Group grp1(p, "geometry");
	Flag<float, 3> vec(grp1, {"vec", "v"}, "Vector x y z", {});
	Flag<float, 3> point(grp1, {"point", "p"}, "Point x y z", {1, 2, 3});
	Flag<bool, 0> invert(grp1, "i", "Invert axes.");
	assert(!vec.required);
	assert(!invert.required);
	assert(!invert);

	{
		const char* args[] = {"-h"};
		auto result = p.parse(args, prog);
		assert(result.code == result.help);
	}

	{
		const char* args[] = {"--help"};
		auto result = p.parse(args, prog);
		assert(result.code == result.help);
	}

	{
		const char* args[] = {"3", "2"};
		auto result = p.parse(args, prog);
		assert(result.code == result.unknown);
	}

	{
		const char* args[] = {"-i"};
		auto result = p.parse(args, prog);
		assert(result.code == result.missing);
		*invert = false;
	}

	{
		const char* args[] = {"not an int"};
		auto result = p.parse(args, prog);
		assert(result.code == result.error);
	}

	{
		const char* args[] = {
			"-iv1", "2", "3", "--point", "5", "6", "--1", "-3",
			" -42"
		};
		auto result = p.parse(args, prog);
		assert(result.code == result.success);
		assert(invert);
		assert(vec[0] == 1);
		assert(vec[1] == 2);
		assert(vec[2] == 3);
		assert(point[0] == 5);
		assert(point[1] == 6);
		assert(point[2] == -3);
		assert(*num == -42);
		assert(result.parsed(vec));
		assert(result.parsed(point));
		assert(result.parsed(invert));
		assert(result.parsed(num));
	}

	return 0;
}

int repeat1(const char *prog)
{
	using namespace argparse;
	Parser p("repeat flag1");
	Flag<int> f1(p, {"f", "flag1"}, "first flag", {});
	try
	{
		Flag<int> f2(p, {"f", "flag2"}, "second flag", 0);
		assert(false);
	}
	catch(std::logic_error &e)
	{}

	Group g(p, "group1");
	try
	{
		Flag<float, 3> f2(p, {"f", "flag2"}, "second flag in group");
		assert(false);
	}
	catch(std::logic_error &e)
	{}
	return 0;
}

int repeat2(const char *prog)
{
	using namespace argparse;
	Parser p("repeat pos");
	Arg<float, 2> f1(p, "xy", "xy coordinate");
	Arg<float, 2> f2(p, "xy", "xy coordinate");
	const char * args[] = {
		"1", "2", "3", "4"
	};
	try
	{
		auto result = p.parse(args, prog);
		assert(false);
	}
	catch (std::logic_error &e)
	{}
	return 0;
}

// considerations:
// arg / flag
// required / not required
// single / fixed / variable args / toggle / count
//
//
int full(const char *prog)
{


	return 0;
}

int main(int argc, char *argv[])
{
	return (
		basics(argv[0])
		|| repeat1(argv[0])
		|| full(argv[0])
	);
}
