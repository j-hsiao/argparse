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
		int forcepos;

		ArgIter(int argc, const char * const argv[], const char *prefix="-"):
			isflag(0),
			argc(argc),
			pos(-1),
			argv(argv),
			prefix(prefix),
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
		{ return isflag && argv[pos][isflag] == '0' && !argv[pos][isflag+1]; }

		//step to the next arg.
		void step()
		{
			++pos;
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
				const char *remain = argv[pos] + isflag;
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
			{ isflag = 0; }
		}

		//name of flag without prefix chars
		const char* flag() const
		{
			if (isflag) { return argv[pos] + isflag; }
			else { return nullptr; }
		}

		//current argument
		const char* arg() const { return argv[pos]; }
	};
}
#endif //ARGPARSE_ARGITER_HPP
