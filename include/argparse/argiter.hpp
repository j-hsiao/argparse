// Iterate on arguments.
// special arguments (assume prefix = '-'):
//   --     Treat all remaining arguments as positional.
//   --N    N: a number, treat the next N arguments as positional.
//          (no trailing characters after the number)
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
		const char **argv;
		const char *prefix;
		int forcepos;


		ArgIter(int argc, const char *argv[], const char *prefix="-"):
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
					{ step(); }
					else
					{ forcepos = 0; }
				}
				else
				{ forcepos = -1; }
			}
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
