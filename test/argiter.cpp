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
		assert(!it.isflag);
		assert(it.arg == args[0]);
		it.step();
		assert(!std::strcmp(it.arg, "world"));
		assert(!it.isflag);
		assert(it.arg == args[1]);
		it.step();
		assert(!std::strcmp(it.arg, "--69"));
		assert(it.arg == args[3]);
		assert(!it.isflag);
		it.step();
		assert(!std::strcmp(it.arg, "whatever"));
		assert(!it.isflag);
		assert(it.arg == args[4]);
		it.step();
		assert(!std::strcmp(it.arg, "h"));
		assert(it.arg == args[5]+1);
		it.step();
		assert(!std::strcmp(it.arg, "verbose"));
		assert(it.arg == args[6]+2);
		it.step();
		assert(!std::strcmp(it.arg, "2"));
		assert(it.arg == args[7]+1);
		it.step();
		assert(!std::strcmp(it.arg, "-flag1"));
		assert(!it.isflag);
		assert(it.arg == args[9]);
		it.step();
		assert(!std::strcmp(it.arg, "--flag2"));
		assert(!it.isflag);
		assert(it.arg == args[10]);
		it.step();
		assert(!std::strcmp(it.arg, "flag3"));
		assert(it.arg == args[11]+2);
	}

	{
		using namespace argparse;
		std::vector<const char*> args{
			"pos1", "-czfasdf", "--1", "-3", "-tzf", "asdf",
			"-", "--0", "-----0"
		};

		ArgIter it(args.size(), &args[0], "-");
		assert(it);
		assert(!std::strcmp(it.arg, "pos1"));
		it.step();
		assert(it);
		assert(!std::strcmp(it.arg, "czfasdf"));
		assert(it.isflag == 1);
		assert(it.arg[0] == 'c');
		it.stepflag();
		assert(it);
		assert(!std::strcmp(it.arg, "zfasdf"));
		it.stepflag();
		assert(it);
		assert(!std::strcmp(it.arg, "fasdf"));
		it.stepflag();
		assert(it);
		assert(!std::strcmp(it.arg, "asdf"));
		it.step();
		assert(it);
		assert(!std::strcmp(it.arg, "-3"));
		assert(!it.isflag);
		it.step();
		assert(it);
		assert(it.isflag == 1);
		assert(it.arg[0] == 't');
		it.stepflag();
		assert(it);
		assert(it.arg && it.arg[0] == 'z');
		it.stepflag();
		assert(it);
		assert(it.arg && it.arg[0] == 'f');
		it.stepflag();
		assert(it);
		assert(!std::strcmp(it.arg, "asdf"));
		assert(it);
		it.step();
		assert(it);
		assert(!std::strcmp(it.arg, "-"));
		assert(!it.isflag);
		it.step();
		assert(it);
		assert(it.breakpoint());
		assert(it.isflag);
		assert(!std::strcmp(it.arg, "0"));

		it.stepbreak();
		assert(it.isflag);
		assert(it);
		assert(it.breakpoint());
		assert(!std::strcmp(it.arg, "---0"));

		it.stepbreak();
		assert(it);
		assert(it.breakpoint());

		it.stepbreak();
		assert(it);
		assert(it.breakpoint());

		it.stepbreak();
		assert(it);
		assert(it.breakpoint());

		it.stepbreak();
		assert(!it);
	}

	return 0;
}
