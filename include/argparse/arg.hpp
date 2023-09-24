// Argument classes.
//
// Arg instances should match the interface of the wrapped argument type
// as close as possible.
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
#include "argparse/parse.hpp"
#include "argparse/printable.hpp"

#include <array>
#include <utility>
#include <vector>
#include <stdexcept>
#include <string>
#include <ostream>
#include <type_traits>

namespace argparse
{
	struct ArgCommon
	{
		const char * const help;
		bool required;

		template<class Parser>
		ArgCommon(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, bool required
		):
			help(help),
			required(required)
		{
			//TODO: register with p using names
		}

		virtual bool parse(ArgIter &it) = 0;
		virtual void print_count(std::ostream &o) const = 0;
		virtual void print_defaults(std::ostream &o) const = 0;
	};

	//Fixed num of multiple arguments
	template<class T, int N>
	struct FixedArgs: public ArgCommon
	{
		std::array<T, N> data;

		template<class Parser>
		FixedArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			ArgCommon(p, names, help, true),
			data{}
		{}

		template<class Parser, int M>
		FixedArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const T (&defaults)[M]
		):
			ArgCommon(p, names, help, false),
			data{}
		{
			static_assert(N == M, "Fixed multi-arg count does not match number of defaults.");
			for (int idx=0; idx<N; ++idx)
			{ data[idx] = defaults[idx]; }
		}

		virtual bool parse(ArgIter &it) override
		{
			for (int i = 0; i < N; ++i)
			{ if (!argparse::parse(data[i], it)) { return false; } }
			return true;
		}

		virtual void print_count(std::ostream &o) const override
		{ o << " x" << N; }
	};

	//Variable arguments
	template<class T>
	struct VarArgs: public ArgCommon
	{
		typedef std::vector<T> defaults_type;
		std::vector<T> data;

		template<class Parser>
		VarArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			ArgCommon(p, names, help, true),
			data{}
		{}

		template<class Parser>
		VarArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const defaults_type &defaults
		):
			ArgCommon(p, names, help, false),
			data(defaults.begin(), defaults.end())
		{}

		virtual bool parse(ArgIter &it) override
		{
			data.clear();
			T tmp;
			while (argparse::parse(tmp, it))
			{ data.push_back(tmp); }
			return true;
		}

		virtual void print_count(std::ostream &o) const override
		{ o << " ..."; }
	};

	template<class T>
	struct SingleArg: public ArgCommon
	{
		typedef T defaults_type;
		T data;

		template<class Parser>
		SingleArg(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			ArgCommon(p, names, help, true)
		{}

		template<class Parser>
		SingleArg(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const T &defaults
		):
			ArgCommon(p, names, help, false),
			data(defaults)
		{}

		virtual bool parse(ArgIter &it) override
		{ return argparse::parse(data, it); }

		virtual void print_count(std::ostream &o) const override { o << " x1"; }
	};

	struct ToggleBool: public ArgCommon
	{
		typedef bool defaults_type;
		bool data;

		template<class Parser>
		ToggleBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			ArgCommon(p, names, help, true),
			data(false)
		{}

		template<class Parser>
		ToggleBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const defaults_type &defaults
		):
			ArgCommon(p, names, help, false),
			data(defaults)
		{}

		virtual bool parse(ArgIter &it) override
		{
			data = !data;
			return true;
		}

		virtual void print_count(std::ostream &o) const override
		{ o << "!!"; }
	};

	struct CountBool: public ArgCommon
	{
		typedef int defaults_type;
		int data;

		template<class Parser>
		CountBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			ArgCommon(p, names, help, true),
			data(0)
		{}

		template<class Parser>
		CountBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const defaults_type &defaults
		):
			ArgCommon(p, names, help, false),
			data(defaults)
		{}

		virtual bool parse(ArgIter &it) override
		{
			++data;
			return true;
		}

		virtual void print_count(std::ostream &o) const override
		{ o << "++"; }
	};

	template<class T, int N>
	struct Impl
	{ typedef VarArgs<T> impl; };

	template<class T>
	struct Impl<T, 1>
	{ typedef SingleArg<T> impl; };

	template<>
	struct Impl<bool, 1>
	{ typedef CountBool impl; };

	template<>
	struct Impl<bool, 0>
	{ typedef ToggleBool impl; };

	template<class T, int N, bool fixed=(N>1)>
	struct BasicArg: public Impl<T, N>::impl
	{
		typedef typename Impl<T, N>::impl impl_type;

		using impl_type::impl_type;

		template<class Parser>
		BasicArg(
			Parser &p, const char *name,
			const char *help=nullptr
		):
			impl_type(p, {name}, help)
		{}

		template<class Parser>
		BasicArg(
			Parser &p, const char *name,
			const char *help, const typename impl_type::defaults_type &defaults
		):
			impl_type(p, {name}, help, defaults)
		{}
	};

	template<class T, int N>
	struct BasicArg<T, N, true>: public FixedArgs<T, N>
	{
		using FixedArgs<T, N>::FixedArgs;

		template<class Parser>
		BasicArg(
			Parser &p, const char *name,
			const char *help=nullptr
		):
			FixedArgs<T, N>(p, {name}, help)
		{}

		template<class Parser, int M>
		BasicArg(
			Parser &p, const char *name,
			const char *help, const T (&defaults)[M]
		):
			FixedArgs<T, N>(p, {name}, help, defaults)
		{}
	};

	template<class impl>
	struct Wrapper: public impl
	{
		typedef decltype(impl::data) data_type;

		using impl::impl;

		data_type& operator*() { return this->data; }
		const data_type& operator*() const { return this->data; }

		data_type* operator->() { return &this->data; }
		const data_type* operator->() const { return &this->data; }
	};

	template<class T, class V>
	void operator<<(T&&, V&&);

	template<class T, int N=1, bool multi=(N<0||N>1)>
	struct Arg: public Wrapper<BasicArg<T, N>>
	{
		typedef Wrapper<BasicArg<T, N>> par;
		using par::par;

		T& operator[](std::size_t idx)
		{ return this->data[idx]; }

		const T& operator[](std::size_t idx) const
		{ return this->data[idx]; }

		virtual void print_defaults(std::ostream &o) const override
		{
			if (!check::Printable<T>::value) { return; }
			o << '[';
			auto start = this->data.begin();
			auto stop = this->data.end();
			if (start != stop)
			{
				o << *start;
				++start;
			}
			for (; start != stop; ++start)
			{ o << ", " << *start; }
			o << ']';
		}
	};

	template<class T, int N>
	struct Arg<T, N, false>: public Wrapper<BasicArg<T, N>>
	{
		typedef Wrapper<BasicArg<T, N>> par;
		using par::par;

		virtual void print_defaults(std::ostream &o) const override
		{
			if (!check::Printable<T>::value) { return; }
			o << this->data;
		}
	};

	template<class T, int N>
	struct Flag: public Arg<T, N>
	{ using Arg<T, N>::Arg; };


}
#endif //ARGPARSE_ARG_HPP
