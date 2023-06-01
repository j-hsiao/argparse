// Argument types.
//
// To create new types:
//   1. specialize TypedArg<type, count>.
//   2. Inherit from Arg.
//   3. data member should be returned and fill() should fill
//      it by parsing from args.
//
// TypedArg is implemented for basic integral types and char*.
//
// Having defaults makes arg optional
// (if not optional, what's the point of the defaults?)
#ifndef ARGPARSE_ARG_HPP
#define ARGPARSE_ARG_HPP

#include "argparse/argiter.hpp"
#include "argparse/nums.hpp"
#include <array>
#include <utility>
#include <vector>
#include <stdexcept>
#include <ostream>

namespace argparse
{
	struct Arg
	{
		const char *name;
		const char *help;
		bool parsed;
		Arg(const char *name, const char *help, bool parsed=false):
			name(name), help(help), parsed(parsed)
		{}

		virtual bool fill(ArgIter &it) = 0;

		virtual void argspec(std::ostream &o) const = 0;
		virtual void defaults(std::ostream &o) const = 0;

	};

	std::ostream& operator<<(std::ostream &o, const Arg &arg)
	{
		o << arg.name << ' ';
		arg.argspec(o);
		return o;
	}


	template<class T>
	int fill(T *data, int count, ArgIter &it)
	{
		for (int i=0; i<count; ++i)
		{
			if (it.isflag || !store(data[i], it.arg()))
			{ return i; }
			it.step();
		}
		return count;
	}
	template<class T>
	void printvals(std::ostream &o, const T &data)
	{
		o << '[' << data[0];
		for (int i=0; i<data.size(); ++i)
		{ o << ", " << data[i]; }
		o << ']';
	}

	//Fixed count args
	template <class T, int count=1>
	struct TypedArg: public Arg
	{
		std::array<T, count> data;
		bool defaulted;

		TypedArg(const char *name, const char *help, std::initializer_list<T> defaults={}):
			Arg(name, help, defaults.size() == count),
			data{},
			defaulted(defaults.size())
		{
			if (defaults.size())
			{
				if (defaults.size() != count)
				{ throw std::logic_error("The number of defaults should match the count."); }
				int i = 0;
				for (auto &item: defaults)
				{
					data[i] = item;
					++i;
				}
			}
		}

		void argspec(std::ostream &o) const override
		{ o << "x" << count; }
		void defaults(std::ostream &o) const override
		{
			if (defaulted) { printvals(o, data); }
		}

		bool fill(ArgIter &it) override
		{ return argparse::fill(&data[0], count, it) == count; }
	};

	//Multiple args
	template<class T>
	struct TypedArg<T, -1>: public Arg
	{
		std::vector<T> data;
		TypedArg(const char *name, const char *help, std::initializer_list<T> defaults={}):
			Arg(name, help, true),
			data(defaults)
		{
			if (defaults.size())
			{ data.assign(defaults.begin(), defaults.end()); }
		}

		bool fill(ArgIter &it) override
		{
			data.clear();
			while (1)
			{
				T item;
				if (it.isflag || !store(item, it.arg()))
				{ return true; }
				data.push_back(item);
				it.step();
			}
			return true;
		}

		void argspec(std::ostream &o) const override
		{ o << "..."; }
		void defaults(std::ostream &o) const override
		{ printvals(o, data); }
	};

	//counting flags
	template<>
	struct TypedArg<bool, 1>: public Arg
	{
		int data;
		//ignore defaults.
		//counting so if defaults then this is always true...
		TypedArg(const char *name, const char *help, std::initializer_list<bool> defaults={}):
			Arg(name, help, true),
			data(0)
		{}
		bool fill(ArgIter &it) override { ++data; return true; }
		void argspec(std::ostream &o) const override { o << "++"; }
		void defaults(std::ostream &o) const override
		{ o << data; }
	};

	//toggle flag
	template<>
	struct TypedArg<bool, 0>: public Arg
	{
		bool data;
		//ignore defaults.
		//counting so if defaults then this is always true...
		TypedArg(const char *name, const char *help, std::initializer_list<bool> defaults={}):
			Arg(name, help, true),
			data(0)
		{
			if (defaults.size())
			{ data = *defaults.begin(); }
		}
		bool fill(ArgIter &it) override { data = !data; return true; }
		void argspec(std::ostream &o) const override { o << "!!"; }
		void defaults(std::ostream &o) const override
		{ o << data; }
	};


}
#endif //ARGPARSE_ARG_HPP
