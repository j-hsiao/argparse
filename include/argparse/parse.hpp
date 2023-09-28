// Parse objects from ArgIters.
// Return of 0 is failure.
// Return of 1 is good, continue parsing.
// Return of 2 is good, but stop parsing.
//   This is mainly used to prevent infinite parsing of nested vectors.
//   For example, a vector of vector of ints,
//   args: 1 2 3 hello
//     first vector = [1, 2, 3]
//     second vector = []
//     third vector = []
//     ...
//
#ifndef ARGPARSE_PARSE_HPP
#define ARGPARSE_PARSE_HPP

#include "argparse/nums.hpp"
#include "argparse/argiter.hpp"

#include <array>
#include <vector>
#include <utility>

#include <iostream>
namespace argparse
{
	template<class T, std::size_t N>
	int parse(std::array<T, N> &v, ArgIter &it)
	{
		for (int i=0; i<N; ++i)
		{ if (!parse(v[i], it)) { return 0; } }
		return 1;
	}

	template<class T>
	int parse(std::vector<T> &v, ArgIter &it)
	{
		v.clear();
		T tmp;
		while (it)
		{
			if (it.breakpoint())
			{
				it.stepbreak();
				return 1;
			}
			else if (int code = parse(tmp, it))
			{
				v.push_back(std::move(tmp));
				if (code == 2)
				{ return 2; }
			}
			else
			{ return 2; }
		}
		return 2;
	}

	template<class T>
	int parse(T &out, ArgIter &it)
	{
		if (it.isarg() && store(out, it.arg))
		{
			it.step();
			return 1;
		}
		return 0;
	}

	int parse(const char * &out, ArgIter &it);

	//Allow use of these parse methods as fallbacks.
	template<class T>
	bool adl_parse(T &out, ArgIter &it)
	{ return parse(out, it); }
}
#endif //ARGPARSE_PARSE_HPP
