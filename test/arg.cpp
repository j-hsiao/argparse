#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"

#undef NDEBUG
#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <array>
#include <vector>

struct Point { int x, y; };
bool operator==(const Point &a, const Point &b)
{ return a.x == b.x && a.y == b.y; }

namespace argparse
{
	template<>
	bool parse(Point &dst, ArgIter &it)
	{ return parse(dst.x, it) && parse(dst.y, it); }
}
std::ostream& operator<<(std::ostream &o, const Point &p)
{
	o << '<' << p.x << ',' << p.y << '>';
	return o;
}

struct DummyParser
{
	//TODO dummy interface
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
	}
	it.reset();
	{
		std::initializer_list<Point> fk{{1,2}, {3,4}};
		argparse::Arg<Point, -1> mypoints(dummy, "points", "2 points", {{1,2}, {3,4}});
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
	}
	it.reset();
	{
		argparse::Arg<Point> mypoint(dummy, "point", "x y");
		mypoint.parse(it);
		assert(!std::strcmp(it.arg, "8"));
		assert(mypoint->x == 6);
		assert(mypoint->y == 9);

		mypoint.parse(it);
		assert(!it);
		assert(mypoint->x == 8);
		assert(mypoint->y == 4);
		assert(*mypoint == *mypoint);
	}
	{
		argparse::Arg<bool> countbool(dummy, "count", nullptr);
	}


	return 0;
}
