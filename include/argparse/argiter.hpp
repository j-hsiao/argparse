// Iterate on arguments.
// special arguments (assume prefix = '-'):
//   --     Treat all remaining arguments as non-flags.
//   --N    N: a number, treat the next N arguments as non-flags.
//          (no trailing characters after the number)
//   --0    Explicitly break an argument sequence.  This acts like a
//          flag, but it has no meaning.  Users should call breakpoint()
//          to check if the argument is a flag or is just this
//          breakpoint flag.
//
// Iteration can be by argument or by shortflag.
//
#ifndef ARGPARSE_ARGITER_HPP
#define ARGPARSE_ARGITER_HPP
#include <cstddef>
#include <cstring>
#include <cstdlib>

namespace argparse
{
	struct ArgIter
	{
		//> 0 if isflag else not a flag (doesn't start with prefix)
		std::size_t isflag;
		int argc, pos;
		const char * const *argv;
		const char *prefix;
		const char *arg;
		int forcepos;

		ArgIter(int argc, const char * const argv[], const char *prefix="-"):
			isflag(0),
			argc(argc),
			pos(-1),
			argv(argv),
			prefix(prefix),
			arg(nullptr),
			forcepos(0)
		{ step(); }

		operator bool() const { return pos < argc; }

		void reset()
		{
			pos = -1;
			forcepos = 0;
			step();
		}

		bool breakpoint() const
		{ return isflag == 2 && argv[pos][2] == '0' && !argv[pos][3]; }

		//name of flag without prefix chars
		const char* flag() const
		{
			if (isflag > 2)
			{ return argv[pos] + 2; }
			else if (isflag)
			{ return argv[pos] + isflag; }
			else
			{ return nullptr; }
		}

		//Assume the current arg is a short flag.
		//Step through the current short-flag position
		//If the current arg is exhausted, then step
		//to the next arg.
		void stepflag()
		{
			if (arg[0] == prefix[0])
			{ arg = flag() + 1; }
			else
			{ ++arg; }
			if (!arg[0]) { step(); }
		}

		//step to the next arg.
		void step()
		{
			++pos;
			arg = argv[pos];
			if (pos >= argc) { return; }
			if (forcepos)
			{
				--forcepos;
				isflag = 0;
				return;
			}
			isflag = std::strspn(argv[pos], prefix);
			if (isflag == 2)
			{
				const char *remain = argv[pos] + 2;
				if (remain[0])
				{
					char *end;
					forcepos = static_cast<int>(std::strtoll(remain, &end, 10));
					//outof range gives acceptable values. (skip all remaining)
					if (!end[0])
					{ if (forcepos) { step(); } }
					else
					{ forcepos = 0; }
				}
				else
				{
					forcepos = -1;
					step();
				}
			}
			else if (!argv[pos][isflag])
			{
				//Allow '-' as an arg since it is the general arg to indicate
				//pipe.
				isflag = 0;
			}
		}
	};
}
#endif //ARGPARSE_ARGITER_HPP
