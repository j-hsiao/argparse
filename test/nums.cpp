#include "argparse/nums.hpp"

#include <cassert>

int main(int argc, char *argv[])
{
	int val;
	assert(argparse::store(val, "1234") && val == 1234);
	assert(argparse::store(val, "  1243") && val == 1243);
	assert(argparse::store(val, "1324  ") && val == 1324);
	assert(argparse::store(val, "  1342  ") && val == 1342);
	assert(argparse::store(val, "-1423") && val == -1423);
	assert(argparse::store(val, "  -1432") && val == -1432);
	assert(argparse::store(val, "-2134  ") && val == -2134);
	assert(argparse::store(val, "  -2143  ") && val == -2143);
	assert(!argparse::store(val, "    "));
	assert(!argparse::store(val, "  hi  "));
	assert(!argparse::store(val, "hi  "));
	assert(!argparse::store(val, "  hi"));
	assert(!argparse::store(val, "  69hi"));
	return 0;
}
