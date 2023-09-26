//Convert commandline args into some object.

#include "argparse/nums.hpp"
#include "argparse/argiter.hpp"

namespace argparse
{
	template<class T>
	bool parse(T &out, ArgIter &it)
	{
		if (it.isarg() && store(out, it.arg))
		{
			it.step();
			return true;
		}
		return false;
	}

	template<>
	inline bool parse<const char*>(const char * &out, ArgIter &it)
	{
		if (it.isarg())
		{
			out = it.arg;
			it.step();
			return true;
		}
		return false;
	}

	template<class T>
	bool adl_parse(T &out, ArgIter &it)
	{ return parse(out, it); }
}
