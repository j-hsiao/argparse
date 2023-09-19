#undef NDEBUG
#include <iostream>
#include "argparse/argiter.hpp"
#include <cstdarg>
#include <cstring>
#include <vector>
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
		assert(!std::strcmp(it.arg, "hello"));
		assert(!it.flag());
		assert(it.pos == 0);
		it.step();
		assert(!std::strcmp(it.arg, "world"));
		assert(!it.flag());
		assert(it.pos == 1);
		it.step();
		assert(!std::strcmp(it.arg, "--69"));
		assert(it.pos == 3);
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg, "whatever"));
		assert(!it.flag());
		assert(it.pos == 4);
		it.step();
		assert(!std::strcmp(it.arg, "-h"));
		assert(!std::strcmp(it.flag(), "h"));
		assert(it.pos == 5);
		it.step();
		assert(!std::strcmp(it.arg, "--verbose"));
		assert(!std::strcmp(it.flag(), "verbose"));
		assert(it.pos == 6);
		it.step();
		assert(!std::strcmp(it.arg, "-2"));
		assert(!std::strcmp(it.flag(), "2"));
		assert(it.pos == 7);
		it.step();
		assert(!std::strcmp(it.arg, "-flag1"));
		assert(!it.flag());
		assert(it.pos == 9);
		it.step();
		assert(!std::strcmp(it.arg, "--flag2"));
		assert(!it.flag());
		assert(it.pos == 10);
		it.step();
		assert(!std::strcmp(it.arg, "--flag3"));
		assert(!std::strcmp(it.flag(), "flag3"));
		assert(it.pos == 11);
	}

	{
		using namespace argparse;
		std::vector<const char*> args{
			"pos1", "-czfasdf", "--1", "-3", "-tzf", "asdf"
		};

		ArgIter it(args.size(), &args[0], "-");
		assert(it);
		assert(!std::strcmp(it.arg, "pos1"));
		it.step();
		assert(!std::strcmp(it.arg, "-czfasdf"));
		assert(it.isflag == 1);
		assert(it.flag()[0] == 'c');
		it.stepflag();
		assert(!std::strcmp(it.arg, "zfasdf"));
		it.stepflag();
		assert(!std::strcmp(it.arg, "fasdf"));
		it.stepflag();
		assert(!std::strcmp(it.arg, "asdf"));
		it.step();
		assert(!std::strcmp(it.arg, "-3"));
		assert(!it.isflag);
		it.step();
		assert(it.isflag == 1);
		assert(it.flag()[0] == 't');
		it.stepflag();
		assert(it.arg && it.arg[0] == 'z');
		it.stepflag();
		assert(it.arg && it.arg[0] == 'f');
		it.stepflag();
		assert(!std::strcmp(it.arg, "asdf"));
	}

	return 0;
}
