#include "argparse/argparse.hpp"
#undef NDEBUG
#include <cassert>

int main(int argc, char *argv[])
{
	argparse::Parser p;

	auto xyz = p.add<int, 3>("xyz", "point coordinates");
	assert(p.pos.size() == 1);
	assert(p.pos[0] == &xyz);
	assert(xyz.required);

	return 0;
}
