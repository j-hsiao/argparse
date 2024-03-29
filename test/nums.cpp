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


	{
		argparse::Base<int, 16> val;
		assert(argparse::store(val, "FF") && val.data == 255);
		assert(val == 255);
		assert(val == val);

		assert(argparse::store(val, "F0") && val.data == 240);
		assert(val == 240);
		assert(val == val);
	}

	{
		argparse::Base<short, 10> val{3};
		assert(val == 3);
		val = 5.67;
		assert(val != 3);
		assert(val == 5);
		assert(val.data == 5);
		argparse::Base<double, 10> val2{3.1415};
		assert(val != val2);
		val2 = 5;
		assert(val == val2);
		assert(val < 10);
	}


	return 0;
}
