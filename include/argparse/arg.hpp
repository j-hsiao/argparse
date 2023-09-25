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
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace argparse
{
	struct ArgCommon;
	struct ArgCount { const ArgCommon *arg; };
	struct FlagCount { const ArgCommon *arg; };
	struct ArgDefaults { const ArgCommon *arg; };
	struct ArgCommon
	{
		std::vector<const char*> names;
		const char * const help;
		bool required;

		ArgCommon() = delete;
		ArgCommon(const ArgCommon&) = delete;
		ArgCommon(ArgCommon&&) = delete;

		template<class Parser>
		ArgCommon(
			Parser *p, std::initializer_list<const char*> names,
			const char *help, bool required
		):
			ArgCommon(names, help, required)
		{
			if (!this->names.size())
			{ throw std::logic_error("Argument requires at least 1 name."); }
			for (const char *name : names)
			{
				if (!name || !name[0])
				{ throw std::logic_error("Arg name should not be null or empty."); }
			}
			if (p) { p->add(*this); }
		}

		ArgCommon(
			std::initializer_list<const char*> names,
			const char *help, bool required
		):
			names(names),
			help(help),
			required(required)
		{}

		virtual bool parse(ArgIter &it) = 0;
		virtual std::ostream& print_count(std::ostream &o) const = 0;
		virtual std::ostream& print_acount(std::ostream &o) const
		{ print_count(o); return o; }
		virtual std::ostream& print_defaults(std::ostream &o) const = 0;
	};

	inline std::ostream& operator<<(std::ostream &o, const ArgCount &c)
	{
		c.arg->print_acount(o);
		return o;
	}
	inline std::ostream& operator<<(std::ostream &o, const FlagCount &c)
	{
		c.arg->print_count(o);
		return o;
	}

	inline std::ostream& operator<<(std::ostream &o, const ArgDefaults &c)
	{
		c.arg->print_defaults(o);
		return o;
	}

	struct FlagCommon: public ArgCommon
	{
		using ArgCommon::ArgCommon;

		template<class Parser>
		FlagCommon(
			Parser *p, std::initializer_list<const char*> names,
			const char *help, bool required
		):
			ArgCommon(names, help, required)
		{ if (p) { p->add(*this); } }
	};

	//Fixed num of multiple arguments
	template<class T, int N, class Base>
	struct FixedArgs: public Base
	{
		std::array<T, N> data;

		template<class Parser>
		FixedArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			Base(&p, names, help, true),
			data{}
		{}

		template<class Parser, int M>
		FixedArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const T (&defaults)[M]
		):
			Base(&p, names, help, false),
			data{}
		{
			static_assert(N == M, "Fixed multi-arg count does not match number of defaults.");
			if (M)
			{
				for (int idx=0; idx<N; ++idx)
				{ data[idx] = defaults[idx]; }
			}
		}

		struct Dummy {};

		template<class Parser>
		FixedArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, Dummy d
		):
			Base(&p, names, help, false),
			data{}
		{}


		virtual bool parse(ArgIter &it) override
		{
			for (int i = 0; i < N; ++i)
			{ if (!argparse::parse(data[i], it)) { return false; } }
			return true;
		}

		virtual std::ostream& print_count(std::ostream &o) const override
		{
			o << " x" << N;
			return o;
		}
	};

	//Variable arguments
	template<class T, class Base>
	struct VarArgs: public Base
	{
		typedef std::vector<T> defaults_type;
		std::vector<T> data;

		template<class Parser>
		VarArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			Base(&p, names, help, true),
			data{}
		{}

		template<class Parser>
		VarArgs(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const defaults_type &defaults
		):
			Base(&p, names, help, false),
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

		virtual std::ostream& printcount(std::ostream &o) const override
		{
			o << " ...";
			return o;
		}
	};

	template<class T, class Base>
	struct SingleArg: public Base
	{
		typedef T defaults_type;
		T data;

		template<class Parser>
		SingleArg(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			Base(&p, names, help, true)
		{}

		template<class Parser>
		SingleArg(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const T &defaults
		):
			Base(&p, names, help, false),
			data(defaults)
		{}

		virtual bool parse(ArgIter &it) override
		{ return argparse::parse(data, it); }

		virtual std::ostream& print_count(std::ostream &o) const override
		{
			o << " x1";
			return o;
		}
		virtual std::ostream& print_acount(std::ostream &o) const override
		{ return o; }
	};

	template<class Base>
	struct ToggleBool: public Base
	{
		typedef bool defaults_type;
		bool data;

		template<class Parser>
		ToggleBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			Base(&p, names, help, false),
			data(false)
		{}

		template<class Parser>
		ToggleBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const defaults_type &defaults
		):
			Base(&p, names, help, false),
			data(defaults)
		{}

		virtual bool parse(ArgIter &it) override
		{
			data = !data;
			return true;
		}

		virtual std::ostream& print_count(std::ostream &o) const override
		{
			o << " !!";
			return o;
		}

		operator bool() const { return data; }
	};

	template<class Base>
	struct CountBool: public Base
	{
		typedef int defaults_type;
		int data;

		template<class Parser>
		CountBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help=nullptr
		):
			Base(&p, names, help, false),
			data(0)
		{}

		template<class Parser>
		CountBool(
			Parser &p, std::initializer_list<const char*> names,
			const char *help, const defaults_type &defaults
		):
			Base(&p, names, help, false),
			data(defaults)
		{}

		virtual bool parse(ArgIter &it) override
		{
			++data;
			return true;
		}

		virtual std::ostream& print_count(std::ostream &o) const override
		 {
			o << " ++";
			return o;
		}

		operator int() const { return data; }
	};

	template<class T, int N, class Base>
	struct Impl
	{ typedef VarArgs<T, Base> impl; };

	template<class T, class Base>
	struct Impl<T, 1, Base>
	{ typedef SingleArg<T, Base> impl; };

	template<class Base>
	struct Impl<bool, 1, Base>
	{ typedef CountBool<Base> impl; };

	template<class Base>
	struct Impl<bool, 0, Base>
	{ typedef ToggleBool<Base> impl; };

	template<class T, int N, class Base, bool fixed=(N>1)>
	struct BasicArg: public Impl<T, N, Base>::impl
	{
		typedef typename Impl<T, N, Base>::impl impl_type;

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

	template<class T, int N, class Base>
	struct BasicArg<T, N, Base, true>: public FixedArgs<T, N, Base>
	{
		using FixedArgs<T, N, Base>::FixedArgs;

		template<class Parser>
		BasicArg(
			Parser &p, const char *name,
			const char *help=nullptr
		):
			FixedArgs<T, N, Base>(p, {name}, help)
		{}

		template<class Parser, int M>
		BasicArg(
			Parser &p, const char *name,
			const char *help, const T (&defaults)[M]
		):
			FixedArgs<T, N, Base>(p, {name}, help, defaults)
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

	template<class T, int N=1, class Base=ArgCommon, bool multi=(N<0||N>1)>
	struct Arg: public Wrapper<BasicArg<T, N, Base>>
	{
		typedef Wrapper<BasicArg<T, N, Base>> par;
		using par::par;

		T& operator[](std::size_t idx)
		{ return this->data[idx]; }

		const T& operator[](std::size_t idx) const
		{ return this->data[idx]; }

		virtual std::ostream& print_defaults(std::ostream &o) const override
		{
			if (this->required || !check::Printable<T>::value) { return o; }
			o << " Default: [";
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
			return o;
		}
	};

	template<class T, int N, class Base>
	struct Arg<T, N, Base, false>: public Wrapper<BasicArg<T, N, Base>>
	{
		typedef Wrapper<BasicArg<T, N, Base>> par;
		using par::par;

		virtual std::ostream& print_defaults(std::ostream &o) const override
		{
			if (this->required || !check::Printable<T>::value) { return o; }
			o << " Default: " << this->data;
			return o;
		}
	};

	template<class T, int N=1, class Base=FlagCommon>
	struct Flag: public Arg<T, N, Base>
	{ using Arg<T, N, Base>::Arg; };

}
#endif //ARGPARSE_ARG_HPP
