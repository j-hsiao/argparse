#undef NDEBUG
#include <iostream>
#include "argparse/argiter.hpp"
#include <cstdarg>
#include <cstring>
#include <vector>
#include <cassert>


int main(int argc, char *argv[])
{
	{
		const char* args[] = {
			"hello", "world", "--1", "--69", "whatever",
			"-h", "--verbose", "-2", "--2", "-flag1",
			"--flag2", "--flag3"
		};

		argparse::ArgIter it(args, "-");
		assert(it);
		assert(!std::strcmp(it.arg, "hello"));
		assert(!it.flag());
		assert(it.arg == args[0]);
		it.step();
		assert(!std::strcmp(it.arg, "world"));
		assert(!it.flag());
		assert(it.arg == args[1]);
		it.step();
		assert(!std::strcmp(it.arg, "--69"));
		assert(it.arg == args[3]);
		assert(!it.flag());
		it.step();
		assert(!std::strcmp(it.arg, "whatever"));
		assert(!it.flag());
		assert(it.arg == args[4]);
		it.step();
		assert(!std::strcmp(it.arg, "h"));
		assert(!std::strcmp(it.flag(), "h"));
		assert(it.arg == args[5]+1);
		it.step();
		assert(!std::strcmp(it.arg, "verbose"));
		assert(!std::strcmp(it.flag(), "verbose"));
		assert(it.arg == args[6]+2);
		it.step();
		assert(!std::strcmp(it.arg, "2"));
		assert(!std::strcmp(it.flag(), "2"));
		assert(it.arg == args[7]+1);
		it.step();
		assert(!std::strcmp(it.arg, "-flag1"));
		assert(!it.flag());
		assert(it.arg == args[9]);
		it.step();
		assert(!std::strcmp(it.arg, "--flag2"));
		assert(!it.flag());
		assert(it.arg == args[10]);
		it.step();
		assert(!std::strcmp(it.arg, "flag3"));
		assert(!std::strcmp(it.flag(), "flag3"));
		assert(it.arg == args[11]+2);
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
		assert(!std::strcmp(it.arg, "czfasdf"));
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
