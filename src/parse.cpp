#include "argparse/parse.hpp"

namespace argparse
{
	int parse(const char* &out, ArgIter &it)
	{
		if (it.isarg())
		{
			out = it.arg;
			it.step();
			return 1;
		}
		return 0;
	}
}
