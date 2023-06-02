// Argument types.
//
// To create new types:
//   1. specialize TypedArg<type, count>.
//   2. Inherit from Arg.
//   3. Have a member "data" that contains the data for the arg
//   4. Constructor: (const char *name, const char *help, std::initializer_list<> defaults, std::vector<Arg*> *vec)
//   5. Move constructor.  This allows the parser to return the actual instance on stack
//      instead of having to allocate.
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
#include <string>
#include <ostream>

namespace argparse
{
	struct Arg
	{
		std::vector<Arg*> *vec;
		const char *name;
		const char *help;
		bool parsed;

		Arg(const char *name, const char *help, std::vector<Arg*> *vec=nullptr, bool parsed=false):
			vec(vec), name(name), help(help), parsed(parsed)
		{
			if (vec) { vec->push_back(this); }
		}
		virtual bool fill(ArgIter &it) = 0;

		virtual void argspec(std::ostream &o) const = 0;
		virtual void defaults(std::ostream &o) const = 0;

		Arg(Arg &&other):
			vec(other.vec),
			name(other.name),
			help(other.help),
			parsed(other.parsed)
		{
			if (vec)
			{
				if (vec->back() == &other)
				{ vec->back() = this; }
				else
				{ vec->push_back(this); }
			}
		}
	};

	std::ostream& operator<<(std::ostream &o, const Arg &arg)
	{
		o << arg.name;
		arg.argspec(o);
		return o;
	}

	template<class TypedArg>
	int fill(TypedArg &inst, int count, ArgIter &it)
	{
		inst.parsed = true;
		for (int i=0; i<count; ++i)
		{
			if (!it || it.isflag || !inst.set(i, it.arg()))
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

	template<class T, int count, bool=(count>=0)>
	struct Datatype
	{
		typedef std::array<T, count> type;
		static void clear(type&){}
		static bool set(type &arr, int i, const char *arg)
		{ return store(arr[i], arg); }
		static void fill(type &arr, const std::initializer_list<T> &l)
		{
			int i = 0;
			for (auto &x : l)
			{ arr[i++] = x; }
		}
	};
	template<class T, int count>
	struct Datatype<T, count, false>
	{
		typedef std::vector<T> type;
		static void clear(type& t){ t.clear(); }
		static bool set(type &arr, int i, const char *arg)
		{
			T item;
			bool result = store(item, arg);
			if (result) { arr.push_back(item); }
			return result;
		}
		static void fill(type &arr, const std::initializer_list<T> &l)
		{ arr.assign(l.begin(), l.end()); }
	};

	//------------------------------
	//General arg impl
	//------------------------------
	template <class T, int count=1>
	struct TypedArg: public Arg
	{
		typedef Datatype<T, count> Typehelp;
		typename Typehelp::type data;
		bool defaulted;

		TypedArg(
			const char *name, const char *help,
			std::initializer_list<T> defaults={}, std::vector<Arg*> *vec=nullptr
		):
			Arg(name, help, vec, count<0 || (defaults.size() == count)),
			data{},
			defaulted(defaults.size())
		{
			if (defaults.size())
			{
				if (count >= 0 && defaults.size() != count)
				{
					throw std::logic_error(
						"The number of defaults should match the count for "
						+ std::string(name));
				}
				Typehelp::fill(data, defaults);
			}
		}

		decltype(data.size()) size() const { return data.size(); }
		T& operator[](std::size_t idx) { return data[idx]; }
		decltype(data.begin()) begin() { return data.begin(); }
		decltype(data.end()) end() { return data.end(); }

		void argspec(std::ostream &o) const override
		{
			if (count < 0) { o << " ..."; }
			else { o << " x" << count; }
		}
		void defaults(std::ostream &o) const override
		{ if (parsed) { printvals(o, data); } }
		bool set(int i, const char *arg)
		{ return Typehelp::set(data, i, arg); }
		bool fill(ArgIter &it) override
		{
			Typehelp::clear(data);
			int numparsed = argparse::fill(
				*this, count<0 ? it.argc : count, it
			);
			return count < 0 || numparsed == count;
		}
	};

	//------------------------------
	//Single arg
	//------------------------------
	template <class T>
	struct TypedArg<T, 1>: public Arg
	{
		T data;
		bool defaulted;

		TypedArg(
			const char *name, const char *help,
			std::initializer_list<T> defaults={}, std::vector<Arg*> *vec=nullptr
		):
			Arg(name, help, vec, defaults.size() == 1),
			data{},
			defaulted(defaults.size())
		{
			if (defaults.size())
			{
				if (defaults.size() != 1)
				{ throw std::logic_error("Only 1 default value should be given to single-typed arg."); }
				data = *defaults.begin();
			}
		}
		operator T() { return data; }

		void argspec(std::ostream &o) const override {}
		void defaults(std::ostream &o) const override
		{ if (defaulted) { o << '(' << data << ')'; } }

		bool fill(ArgIter &it) override
		{
			bool val = it && !it.isflag && store(data, it.arg());
			if (val) { it.step(); }
			return val;
		}
	};

	//------------------------------
	//counting flags
	//------------------------------
	template<>
	struct TypedArg<bool, 1>: public Arg
	{
		int data;
		//ignore defaults.
		//counting so if defaults then this is always true...
		TypedArg(
			const char *name, const char *help,
			std::initializer_list<bool> defaults={}, std::vector<Arg*> *vec=nullptr
		):
			Arg(name, help, vec, true),
			data(0)
		{}
		operator int() { return data; }
		bool fill(ArgIter &it) override { ++data; return true; }
		void argspec(std::ostream &o) const override { o << "++"; }
		void defaults(std::ostream &o) const override
		{ o << data; }
	};

	//------------------------------
	//toggle flag
	//------------------------------
	template<>
	struct TypedArg<bool, 0>: public Arg
	{
		bool data;
		//ignore defaults.
		//counting so if defaults then this is always true...
		TypedArg(
			const char *name, const char *help,
			std::initializer_list<bool> defaults={}, std::vector<Arg*> *vec=nullptr
		):
			Arg(name, help, vec, true),
			data(0)
		{
			if (defaults.size())
			{ data = *defaults.begin(); }
		}
		operator bool() { return data; }
		bool fill(ArgIter &it) override { data = !data; return true; }
		void argspec(std::ostream &o) const override { o << "!!"; }
		void defaults(std::ostream &o) const override
		{ o << data; }
	};


}
#endif //ARGPARSE_ARG_HPP
