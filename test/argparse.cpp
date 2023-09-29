#undef NDEBUG

#include "argparse/argparse.hpp"
#include "argparse/argiter.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>

int basics(const char *prog)
{
	std::stringstream ss;
	using namespace argparse;
	Parser p("Test program", '-', ss);
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
		assert(!result.parsed(num));
		assert(!result.parsed(vec));
		assert(!result.parsed(point));
		assert(!result.parsed(invert));
	}

	{
		const char* args[] = {"--help"};
		auto result = p.parse(args, prog);
		assert(result.code == result.help);
		assert(!result.parsed(num));
		assert(!result.parsed(vec));
		assert(!result.parsed(point));
		assert(!result.parsed(invert));
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

int repeat_flag(const char *prog)
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

int repeat_arg(const char *prog)
{
	using namespace argparse;
	Parser p("repeat pos");
	Arg<float, 2> f1(p, "xy", "xy coordinate");
	Arg<float, 2> f2(p, "xy", "xy coordinate");
	const char * args[] = {
		"1", "2", "3", "4"
	};

	auto result = p.parse(args, prog);
	assert(f1[0] == 1.0f);
	assert(f1[1] == 2.0f);
	assert(f2[0] == 3.0f);
	assert(f2[1] == 4.0f);

	return 0;
}




namespace custom_namespace
{
	struct Point { int x, y; };
	bool parse(Point &out, argparse::ArgIter &it)
	{
		return parse(out.x, it) && parse(out.y, it);
	}
}

int custom(const char *prog)
{
	using namespace argparse;
	Parser p("Custom type.");
	Arg<custom_namespace::Point, 2> point2(
		p, "2 points", "sequence of 2 points x y", {{1,2}, {3,4}});
	assert(point2[0].x == 1);
	assert(point2[0].y == 2);
	assert(point2[1].x == 3);
	assert(point2[1].y == 4);
	{
		const char *args[] = {
			"1000", "1024", "42", "--1", "-1234"};
		auto result = p.parse(args, prog);
		assert(result.code == result.success);
		assert(!result);
		assert(point2[0].x == 1000);
		assert(point2[0].y == 1024);
		assert(point2[1].x == 42);
		assert(point2[1].y == -1234);
	}

	{
		const char *args[] = {
			"1000", "1024", "42", "--1"};
		auto result = p.parse(args, prog);
		assert(result.code == result.error);
		assert(result);
	}

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
	using namespace argparse;
	Parser p("test all the args");

	Arg<int> rsnum(p, "required num", "A required integer");
	Arg<short, 3> rnnum(p, "required 3 nums", "3 required short");
	Arg<long, -1> rvnum(p, "required variable nums", "Variable required long");

	Arg<float> osnum(p, "optional num", "A optional float", {});
	Arg<double, 3> onnum(p, "optional 3 nums", "A optional double", {});
	Arg<const char*, -1> ovnum(p, "optional variable strs", "optional const char*", {});

	Flag<long long> frsnum(p, "rsnum", "A required long long");
	Flag<short, 3> frnnum(p, "rnnum", "3 required short");
	Flag<long, -1> frvnum(p, "rvnum", "Variable required long");

	Flag<float> fosnum(p, "osnum", "A optional float", {});
	Flag<double, 3> fonnum(p, "onnum", "A optional double", {});
	Flag<const char*, -1> fovnum(p, "ovnum", "A optional float", {});

	Flag<bool, 0> toggle(p, {"t", "toggle"}, "toggle a bool");
	Flag<bool> count(p, {"c", "count"}, "count flag instances");

	assert(!toggle.required);
	assert(!count.required);

	{
		const char * args[] = {
			"42", "1", "2", "3", "--0",
			"3.14", "69", "70", "71", "1", "2", "3", "4",
			"--rsnum", "42",
			"--rnnum", "1", "2", "3",
			"--rvnum"
		};
		auto result = p.parse(args, prog);
		assert(result.code == result.success);
		assert(!result);
		assert(result.parsed(rsnum));
		assert(result.parsed(rnnum));
		assert(result.parsed(rvnum));
		assert(result.parsed(osnum));
		assert(result.parsed(onnum));
		assert(result.parsed(ovnum));
		assert(result.parsed(frsnum));
		assert(result.parsed(frnnum));
		assert(result.parsed(frvnum));
		assert(!result.parsed(fosnum));
		assert(!result.parsed(fonnum));
		assert(!result.parsed(fovnum));

		assert(*rsnum == 42);
		assert(rnnum[0] == 1);
		assert(rnnum[1] == 2);
		assert(rnnum[2] == 3);
		assert(rvnum->size() == 0);
		assert(*osnum == 3.14f);
		assert(onnum[0] == 69.0);
		assert(onnum[1] == 70.0);
		assert(onnum[2] == 71.0);
		assert(ovnum->size() == 4);
		assert(!std::strcmp(ovnum[0], "1"));
		assert(!std::strcmp(ovnum[1], "2"));
		assert(!std::strcmp(ovnum[2], "3"));
		assert(!std::strcmp(ovnum[3], "4"));

		assert(*frsnum == 42);
		assert(frnnum[0] == 1);
		assert(frnnum[1] == 2);
		assert(frnnum[2] == 3);
		assert(frvnum->size() == 0);

	}
	return 0;
}

int main(int argc, char *argv[])
{
	return (
		basics(argv[0])
		|| repeat_flag(argv[0])
		|| repeat_arg(argv[0])
		|| full(argv[0])
	);
}
