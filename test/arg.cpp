#include "argparse/arg.hpp"
#include "argparse/argiter.hpp"

#undef NDEBUG
#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>

struct Point { int x, y; };

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
	{
		const char * args[] = {
			"6", "9", "8", "4"
		};
		argparse::Arg<int, 3> mynums(dummy, "nums", "3 ints", {1,2,3});
		assert(!mynums.required);
		assert(mynums[0] == 1);
		assert(mynums[1] == 2);
		assert(mynums[2] == 3);
		argparse::ArgIter it(4, args);
		assert(!std::strcmp(it.arg, "6"));
		mynums.parse(it);
		assert(!std::strcmp(it.arg, "4"));
		assert(mynums[0] == 6);
		assert(mynums[1] == 9);
		assert(mynums[2] == 8);
		it.reset();
		argparse::Arg<Point, 2> mypoints(dummy, "points", "2 points", {{1,2}, {3,4}});
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
	}


	return 0;
}
