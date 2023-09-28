#include "argparse/argiter.hpp"

#include <cstdlib>
#include <cstring>

namespace argparse
{
	ArgIter::ArgIter(int argc, const char * const argv[], const char *prefix):
		isflag(0),
		argc(argc),
		pos(-1),
		argv(argv),
		prefix(prefix),
		arg(nullptr),
		forcepos(0)
	{ step(); }

	void ArgIter::reset()
	{
		pos = -1;
		forcepos = 0;
		step();
	}
	bool ArgIter::isarg() const
	{
		return (
			pos < argc
			&& (!isflag || argv[pos] + isflag < arg));
	}

	bool ArgIter::breakpoint() const
	{ return isflag >= 2 && !std::strcmp(argv[pos]+isflag, "0"); }

	void ArgIter::stepbreak()
	{
		if (arg[0] == '0')
		{ step(); }
		else
		{ ++arg; }
	}

	void ArgIter::stepflag()
	{
		++arg;
		if (!arg[0]) { step(); }
	}
	void ArgIter::step()
	{
		++pos;
		if (pos >= argc) { return; }
		if (forcepos)
		{
			arg = argv[pos];
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
				{
					if (forcepos)
					{
						step();
						return;
					}
				}
				else
				{ forcepos = 0; }
			}
			else
			{
				forcepos = -1;
				step();
			}
		}
		else if (isflag == 1 && !argv[pos][isflag])
		{ isflag = 0; }
		arg = argv[pos] + (isflag >= 2 ? 2 : isflag);
	}
}
