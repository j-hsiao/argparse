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

#include <array>
#include <utility>
#include <vector>
#include <stdexcept>
#include <string>
#include <ostream>

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
		virtual void count(std::ostream &o) = 0;
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

		virtual void count(std::ostream &o) override
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

		virtual void count(std::ostream &o) override
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

		virtual void count(std::ostream &o) override {}
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

		virtual void count(std::ostream &o) override
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

		virtual void count(std::ostream &o) override
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

	template<class T, int N=1, bool multi=(N<0||N>1)>
	struct Arg: public Wrapper<BasicArg<T, N>>
	{
		typedef Wrapper<BasicArg<T, N>> par;
		using par::par;

		T& operator[](std::size_t idx)
		{ return this->data[idx]; }

		const T& operator[](std::size_t idx) const
		{ return this->data[idx]; }
	};

	template<class T, int N>
	struct Arg<T, N, false>: public Wrapper<BasicArg<T, N>>
	{
		typedef Wrapper<BasicArg<T, N>> par;
		using par::par;
	};





//	struct Arg;
//
//	struct ArgRegistry
//	{
//		virtual void push_back(Arg *arg) = 0;
//		virtual Arg*& back() = 0;
//	};
//
//	//The vec arg allows the Arg to be added to a list
//	//without the user having to explicitly add it.
//	//Simplifies user interface.
//	struct Arg
//	{
//		struct Flagfmt { const Arg* arg; };
//		struct Posfmt { const Arg* arg; };
//
//		ArgRegistry *reg;
//		const char *name;
//		const char *help;
//		bool required;
//
//		Arg(const char *name, const char *help, ArgRegistry *reg=nullptr, bool required=false):
//			reg(reg), name(name), help(help), required(required)
//		{ if (reg) { reg->push_back(this); } }
//		//0: success
//		//1: missing arg
//		//2: parse error
//		virtual int fill(ArgIter &it) = 0;
//
//		Flagfmt flag() const { return {this}; }
//		Posfmt pos() const { return {this}; }
//
//		virtual void flagspec(std::ostream &o) const = 0;
//		virtual void posspec(std::ostream &o) const { flagspec(o); }
//		virtual void defaults(std::ostream &o) const = 0;
//
//		Arg(Arg &&other):
//			reg(other.reg),
//			name(other.name),
//			help(other.help),
//			required(other.required)
//		{
//			if (reg)
//			{
//				if (reg->back() == &other) { reg->back() = this; }
//				else { reg->push_back(this); }
//			}
//		}
//	};
//
//	std::ostream& operator<<(std::ostream &o, const Arg::Flagfmt &f)
//	{
//		f.arg->flagspec(o);
//		return o;
//	}
//	std::ostream& operator<<(std::ostream &o, const Arg::Posfmt &p)
//	{
//		p.arg->posspec(o);
//		return o;
//	}
//
//	//------------------------------
//	// creation from ArgIter.
//	//------------------------------
//	// assume it && !it.isflag
//	template<class T>
//	bool create(T &dst, ArgIter &it)
//	{
//		if (store(dst, it.arg()))
//		{
//			it.step();
//			return true;
//		}
//		return false;
//	}
//
//	bool create(const char* &dst, ArgIter &it)
//	{
//		dst = it.arg();
//		it.step();
//		return true;
//	}
//
//	bool create(std::string &dst, ArgIter &it)
//	{
//		dst = it.arg();
//		it.step();
//		return true;
//	}
//
//	//Difference between variable/fixed length multi-arg data.
//	template<class T, int count, bool=(count>=0)>
//	struct MultiDataType
//	{
//		typedef std::array<T, count> type;
//		static void clear(type&){}
//		static bool set(type &arr, int i, ArgIter &it)
//		{ return create(arr[i], it); }
//		static void from_initializer(type &arr, const std::initializer_list<T> &l)
//		{
//			int i = 0;
//			for (auto &x : l) { arr[i++] = x; }
//		}
//	};
//	template<class T, int count>
//	struct MultiDataType<T, count, false>
//	{
//		typedef std::vector<T> type;
//		static void clear(type& t){ t.clear(); }
//		//Assume i is the length of arr
//		static bool set(type &arr, int i, ArgIter &it)
//		{
//			T item;
//			bool result = create(item, it);
//			if (result) { arr.push_back(item); }
//			return result;
//		}
//		static void from_initializer(type &arr, const std::initializer_list<T> &l)
//		{ arr.assign(l.begin(), l.end()); }
//	};
//
//	//Print a list-like container.
//	template<class T>
//	void printvals(std::ostream &o, const T &data)
//	{
//		o << "[";
//		auto it = data.begin();
//		if (it != data.end())
//		{
//			o << *it;
//			++it;
//			while (it != data.end())
//			{
//				o << ", " << *it;
//				++it;
//			}
//		}
//		o << ']';
//	}
//
//	//------------------------------
//	//General arg impl
//	//------------------------------
//	template <class T, int count=1>
//	struct TypedArg: public Arg
//	{
//		typedef MultiDataType<T, count> Typehelp;
//		typename Typehelp::type data;
//		typedef T rawtype;
//		int ndefaults;
//
//		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
//			Arg(name, help, reg, true),
//			data{},
//			ndefaults(0)
//		{}
//
//		TypedArg(
//			const char *name, const char *help,
//			ArgRegistry *reg, std::initializer_list<T> defaults
//		):
//			Arg(name, help, reg, false),
//			data{},
//			ndefaults(defaults.size())
//		{
//			if (ndefaults)
//			{
//				if (count >= 0 && ndefaults != count)
//				{
//					throw std::logic_error(
//						"The number of defaults should match the count for "
//						+ std::string(name));
//				}
//				Typehelp::from_initializer(data, defaults);
//			}
//		}
//		TypedArg(TypedArg &&other):
//			Arg(std::move(static_cast<Arg&>(other))),
//			data(other.data),
//			ndefaults(other.ndefaults)
//		{}
//
//		TypedArg<T, count>& operator=(std::initializer_list<T> vals)
//		{
//			data = vals;
//			return *this;
//		}
//
//		//Where in standard says that this
//		//would cause something like
//		//TypedArg<T,3> x; x[0]
//		//to call T* conversion operator? not sure...
//		operator T*() { return data.data(); }
//		operator const T*() const { return data.data(); }
//		decltype(data)& operator*() { return data; }
//		const decltype(data)& operator*() const { return data; }
//		decltype(data)* operator->() { return &data; }
//		const decltype(data)* operator->() const { return &data; }
//
//		void flagspec(std::ostream &o) const override
//		{
//			o << name;
//			if (count < 0) { o << " ..."; }
//			else { o << " x" << count; }
//		}
//
//		void defaults(std::ostream &o) const override
//		{
//			if (ndefaults || count < 0)
//			{
//				o << "(default: ";
//				printvals(o, data);
//				o << ')';
//			}
//		}
//		int fill(ArgIter &it) override
//		{
//			Typehelp::clear(data);
//			const int num = count > 0 ? count : it.argc;
//			int numparsed = 0;
//			for (; numparsed<num; ++numparsed)
//			{
//				if (!it || it.isflag) { break; }
//				if (!Typehelp::set(data, numparsed, it))
//				{
//					if (count > 0) { return 2; }
//					break;
//				}
//			}
//			if (count < 0)
//			{ if (numparsed == 0 && required) { return 1; } }
//			else
//			{ if (numparsed != count) { return 1; } }
//			return 0;
//		}
//	};
//
//	//------------------------------
//	//Single arg
//	//------------------------------
//	template <class T>
//	struct TypedArg<T, 1>: public Arg
//	{
//		typedef T rawtype;
//		T data;
//		bool defaulted;
//		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
//			Arg(name, help, reg, true),
//			data{},
//			defaulted(false)
//		{}
//
//		TypedArg(
//			const char *name, const char *help,
//			ArgRegistry *reg, std::initializer_list<T> defaults
//		):
//			Arg(name, help, reg, false),
//			data{},
//			defaulted(defaults.size() > 0)
//		{
//			if (defaulted)
//			{
//				if (defaults.size() != 1)
//				{ throw std::logic_error("Only 1 default value should be given to single-typed arg."); }
//				data = *defaults.begin();
//			}
//		}
//		TypedArg(TypedArg &&other):
//			Arg(std::move(static_cast<Arg&>(other))),
//			data(other.data),
//			defaulted(other.defaulted)
//		{}
//
//		T& operator*() { return data; }
//		const T& operator*() const { return data; }
//		T* operator->() { return &data; }
//		const T* operator->() const { return &data; }
//
//		TypedArg<T, 1>& operator=(const T &val)
//		{
//			data = val;
//			return *this;
//		}
//
//		operator T() { return data; }
//		virtual void flagspec(std::ostream &o) const
//		{ o << name << ' ' << name; }
//		virtual void posspec(std::ostream &o) const
//		{ o << name; }
//		void defaults(std::ostream &o) const override
//		{ if (defaulted) { o << "(default: " << data << ')'; } }
//
//		int fill(ArgIter &it) override
//		{
//			if (!it || it.isflag) { return 1; }
//			return create(data, it) ? 0 : 2;
//		}
//	};
//
//	//------------------------------
//	//counting flags
//	//------------------------------
//	template<>
//	struct TypedArg<bool, 1>: public Arg
//	{
//		typedef int rawtype;
//		int data;
//		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
//			Arg(name, help, reg, false),
//			data(0)
//		{}
//
//		TypedArg(
//			const char *name, const char *help, ArgRegistry *reg,
//			std::initializer_list<int> defaults
//		):
//			Arg(name, help, reg, false),
//			data(defaults.size() > 0 ? *defaults.begin() : 0)
//		{
//			if (defaults.size() > 1)
//			{
//				throw std::logic_error(
//					"Bool count " + std::string(name) + " should only have 1 initializer.");
//			}
//		}
//
//		TypedArg(TypedArg &&other):
//			Arg(std::move(static_cast<Arg&>(other))),
//			data(other.data)
//		{}
//
//		TypedArg<bool, 1>& operator=(int val)
//		{
//			data = val;
//			return *this;
//		}
//
//		int& operator*() { return data; }
//		const int& operator*() const { return data; }
//
//		operator int() { return data; }
//		int fill(ArgIter &it) override { ++data; return 0; }
//		void flagspec(std::ostream &o) const override { o << name << "++"; }
//		void defaults(std::ostream &o) const override {}
//	};
//
//	//------------------------------
//	//toggle flag
//	//------------------------------
//	template<>
//	struct TypedArg<bool, 0>: public Arg
//	{
//		typedef bool rawtype;
//		bool data;
//
//		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
//			Arg(name, help, reg, false),
//			data(false)
//		{}
//
//		TypedArg(
//			const char *name, const char *help,
//			ArgRegistry *reg, std::initializer_list<bool> defaults
//		):
//			Arg(name, help, reg, false),
//			data(defaults.size() ? *defaults.begin() : false)
//		{
//			if (defaults.size() > 1)
//			{
//				throw std::logic_error(
//				"Bool flag " + std::string(name) + " should only have 1 initializer.");
//			}
//		}
//		TypedArg(TypedArg &&other):
//			Arg(std::move(static_cast<Arg&>(other))),
//			data(other.data)
//		{}
//
//		TypedArg<bool, 0>& operator=(bool val)
//		{
//			data = val;
//			return *this;
//		}
//
//		bool& operator*() { return data; }
//		const bool& operator*() const { return data; }
//
//		operator bool() { return data; }
//		int fill(ArgIter &it) override { data = !data; return 0; }
//		void flagspec(std::ostream &o) const override { o << name << "!!"; }
//		void defaults(std::ostream &o) const override
//		{ o << "(default: " << (data ? "true" : "false") << ')'; }
//	};


}
#endif //ARGPARSE_ARG_HPP
