#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"

#undef NDEBUG
#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <array>
#include <vector>

namespace mynamespace
{
	struct Point { int x, y; };
	bool operator==(const Point &a, const Point &b)
	{ return a.x == b.x && a.y == b.y; }

	bool parse(Point &dst, argparse::ArgIter &it)
	{ return parse(dst.x, it) && parse(dst.y, it); }
}


struct DummyParser
{
	bool wasflag;
	void add(argparse::ArgCommon&){ wasflag = 0; };
	void add(argparse::FlagCommon&){ wasflag = 1; };
};

int main(int argc, char *argv[])
{
	DummyParser dummy;
	const char * args[] = {
		"6", "9", "8", "4"
	};
	argparse::ArgIter it(4, args);

	{
		argparse::Arg<short, 3> mynums(dummy, "nums", "3 ints", {1,2,3});
		assert(!dummy.wasflag);
		assert(!mynums.required);
		assert(mynums[0] == 1);
		assert(mynums[1] == 2);
		assert(mynums[2] == 3);
		assert(!std::strcmp(it.arg, "6"));
		mynums.parse(it);
		assert(!std::strcmp(it.arg, "4"));
		assert(mynums[0] == 6);
		assert(mynums[1] == 9);
		assert(mynums[2] == 8);
		std::array<short, 3> expect;
		expect[0] = 6;
		expect[1] = 9;
		expect[2] = 8;
		assert(expect == *mynums);
		mynums.print_defaults(std::cout);
		std::cout << std::endl;
		argparse::Arg<const char*, -2> remainder(dummy, "remainder", "the remaining args");
		assert(remainder.parse(it));
		assert(!std::strcmp(remainder->arg, "4"));
	}
	it.reset();
	{
		using namespace mynamespace;
		std::initializer_list<Point> fk{{1,2}, {3,4}};
		argparse::Arg<Point, -1> mypoints(dummy, "points", "2 points", {{1,2}, {3,4}});
		assert(!dummy.wasflag);
		assert(mypoints[0].x == 1);
		assert(mypoints[0].y == 2);
		assert(mypoints[1].x == 3);
		assert(mypoints[1].y == 4);
		assert(!std::strcmp(it.arg, "6"));
		mypoints.parse(it);
		assert(!it);
		assert(mypoints[0].x == 6);
		assert(mypoints[0].y == 9);
		assert(mypoints[1].x == 8);
		assert(mypoints[1].y == 4);
		std::vector<Point> expect = {{6, 9}, {8, 4}};
		assert(*mypoints == expect);
		mypoints.print_defaults(std::cout);
		std::cout << std::endl;
	}
	it.reset();
	{
		using namespace mynamespace;
		argparse::Arg<Point> mypoint(dummy, "point", "x y");
		assert(!dummy.wasflag);
		mypoint.parse(it);
		assert(!std::strcmp(it.arg, "8"));
		assert(mypoint->x == 6);
		assert(mypoint->y == 9);

		mypoint.parse(it);
		assert(!it);
		assert(mypoint->x == 8);
		assert(mypoint->y == 4);
		assert(*mypoint == *mypoint);
		mypoint.print_defaults(std::cout);
		std::cout << std::endl;
	}
	{
		argparse::Flag<bool> countbool(dummy, "count", nullptr);
		assert(dummy.wasflag);
		assert(*countbool == 0);
		countbool.parse(it);
		assert(*countbool == 1);
		countbool.parse(it);
		assert(*countbool == 2);
		countbool.parse(it);
		assert(*countbool == 3);
		countbool.print_defaults(std::cout);
		std::cout << std::endl;
	}
	{
		argparse::Flag<bool, 0> tbool(dummy, "count", nullptr);
		assert(dummy.wasflag);
		assert(!*tbool);
		tbool.parse(it);
		assert(*tbool);
		tbool.parse(it);
		assert(!*tbool);
		tbool.parse(it);
		assert(*tbool);
		tbool.print_defaults(std::cout);
		std::cout << std::endl;
	}

	{
		it.reset();
		argparse::Aflag<int> numadds(dummy, "f", "appended nums");
		assert(numadds.parse(it));
		assert(numadds.data.size() == 1);
		assert(numadds.data[0] == 6);
		assert(numadds.parse(it));
		assert(numadds.data.size() == 2);
		assert(numadds.data[0] == 6);
		assert(numadds.data[1] == 9);
		assert(numadds.parse(it));
		assert(numadds.data.size() == 3);
		assert(numadds.data[0] == 6);
		assert(numadds.data[1] == 9);
		assert(numadds.data[2] == 8);
		assert(numadds.parse(it));
		assert(numadds.data.size() == 4);
		assert(numadds.data[0] == 6);
		assert(numadds.data[1] == 9);
		assert(numadds.data[2] == 8);
		assert(numadds.data[3] == 4);
		std::cout << numadds << std::endl;
	}

	{
		it.reset();
		argparse::Aflag<int, 2> numadds(dummy, "f", "appended nums");
		assert(numadds.parse(it));
		assert(numadds.data.size() == 1);
		assert(numadds.data[0][0] == 6);
		assert(numadds.data[0][1] == 9);
		assert(numadds.parse(it));
		assert(numadds.data.size() == 2);
		assert(numadds.data[0][0] == 6);
		assert(numadds.data[0][1] == 9);
		assert(numadds.data[1][0] == 8);
		assert(numadds.data[1][1] == 4);
		std::cout << numadds << std::endl;
	}

	{
		it.reset();
		argparse::Aflag<int, 2> numadds(dummy, "f", "appended nums", {{1,2}, {3,4}, {5,6}});
		numadds.print_defaults(std::cout) << std::endl;
		assert(numadds.parse(it));
		assert(numadds.data.size() == 1);
		assert(numadds.data[0][0] == 6);
		assert(numadds[0][1] == 9);
		assert(numadds.parse(it));
		assert(numadds->size() == 2);
		assert(numadds[0][0] == 6);
		assert(numadds[0][1] == 9);
		assert(numadds[1][0] == 8);
		assert(numadds[1][1] == 4);
		std::cout << numadds << std::endl;
	}

	{
		it.reset();
		argparse::Arg<std::array<int, 2>, -1> numadds(dummy, "f", "appended pairs");
		assert(numadds.parse(it));
		assert(numadds.data.size() == 2);
		assert(numadds.data[0][0] == 6);
		assert(numadds.data[0][1] == 9);
		assert(numadds.data[1][0] == 8);
		assert(numadds.data[1][1] == 4);
		std::cout << numadds << std::endl;
	}

	{
		const char *args[] = {
			"1", "2", "3", "--0", "5", "6", "7", "8", "---0", "asdf"
		};
		argparse::ArgIter it(args, "-");
		argparse::Arg<std::vector<int>,-1> vv(dummy, "f", "list of lists of ints", {{3,2}, {1}});

		assert(vv[0].size() == 2);
		assert(vv[1].size() == 1);
		assert(vv[0][0] == 3);
		assert(vv[0][1] == 2);
		assert(vv[1][0] == 1);
		std::cout << vv << std::endl;

		assert(it);
		assert(vv.parse(it));
		assert(vv->size() == 2);
		assert(vv[0].size() == 3);
		assert(vv[1].size() == 4);
		assert(vv[0][0] == 1);
		assert(vv[0][1] == 2);
		assert(vv[0][2] == 3);
		assert(vv[1][0] == 5);
		assert(vv[1][1] == 6);
		assert(vv[1][2] == 7);
		assert(vv[1][3] == 8);

		assert(it);
		assert(!std::strcmp(it.arg, "asdf"));

		std::cout << vv << std::endl;
	}
	return 0;
}
