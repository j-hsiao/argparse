#include <iostream>
#include "argparse/argiter.hpp"
#include <cstdarg>
#include <cstring>
#include <vector>
#undef NDEBUG
#include <cassert>

#include "argstruct.hpp"


int main(int argc, char *argv[])
{
	{
		auto myargs = args(
			"hello", "world", "--1", "--69", "whatever", "-h", "--verbose",
			"-2", "--2", "-flag1", "--flag2", "--flag3"
		);

		argparse::ArgIter it(myargs.size(), myargs.args, "-");
		assert(it);
		assert(!std::strcmp(it.arg(), "hello"));
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg(), "world"));
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg(), "--69"));
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg(), "whatever"));
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg(), "-h"));
		assert(!std::strcmp(it.flag(), "h"));
		it.step();
		assert(!std::strcmp(it.arg(), "--verbose"));
		assert(!std::strcmp(it.flag(), "verbose"));
		it.step();
		assert(!std::strcmp(it.arg(), "-2"));
		assert(!std::strcmp(it.flag(), "2"));
		it.step();
		assert(!std::strcmp(it.arg(), "-flag1"));
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg(), "--flag2"));
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg(), "--flag3"));
		assert(!std::strcmp(it.flag(), "flag3"));

	}

	return 0;
}
