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
	struct Arg;

	struct ArgRegistry
	{
		virtual void push_back(Arg *arg) = 0;
		virtual Arg*& back() = 0;
	};

	//The vec arg allows the Arg to be added to a list
	//without the user having to explicitly add it.
	//Simplifies user interface.
	struct Arg
	{
		struct Flagfmt { const Arg* arg; };
		struct Posfmt { const Arg* arg; };

		ArgRegistry *reg;
		const char *name;
		const char *help;
		bool required;

		Arg(const char *name, const char *help, ArgRegistry *reg=nullptr, bool required=false):
			reg(reg), name(name), help(help), required(required)
		{ if (reg) { reg->push_back(this); } }
		//0: success
		//1: missing arg
		//2: parse error
		virtual int fill(ArgIter &it) = 0;

		Flagfmt flag() const { return {this}; }
		Posfmt pos() const { return {this}; }

		virtual void flagspec(std::ostream &o) const = 0;
		virtual void posspec(std::ostream &o) const { flagspec(o); }
		virtual void defaults(std::ostream &o) const = 0;

		Arg(Arg &&other):
			reg(other.reg),
			name(other.name),
			help(other.help),
			required(other.required)
		{
			if (reg)
			{
				if (reg->back() == &other) { reg->back() = this; }
				else { reg->push_back(this); }
			}
		}
	};

	std::ostream& operator<<(std::ostream &o, const Arg::Flagfmt &f)
	{
		f.arg->flagspec(o);
		return o;
	}
	std::ostream& operator<<(std::ostream &o, const Arg::Posfmt &p)
	{
		p.arg->posspec(o);
		return o;
	}

	//------------------------------
	// creation from ArgIter.
	//------------------------------
	// assume it && !it.isflag
	template<class T>
	bool create(T &dst, ArgIter &it)
	{
		if (store(dst, it.arg()))
		{
			it.step();
			return true;
		}
		return false;
	}

	bool create(const char* &dst, ArgIter &it)
	{
		dst = it.arg();
		it.step();
		return true;
	}

	bool create(std::string &dst, ArgIter &it)
	{
		dst = it.arg();
		it.step();
		return true;
	}

	//Difference between variable/fixed length multi-arg data.
	template<class T, int count, bool=(count>=0)>
	struct MultiDataType
	{
		typedef std::array<T, count> type;
		static void clear(type&){}
		static bool set(type &arr, int i, ArgIter &it)
		{ return create(arr[i], it); }
		static void from_initializer(type &arr, const std::initializer_list<T> &l)
		{
			int i = 0;
			for (auto &x : l) { arr[i++] = x; }
		}
	};
	template<class T, int count>
	struct MultiDataType<T, count, false>
	{
		typedef std::vector<T> type;
		static void clear(type& t){ t.clear(); }
		//Assume i is the length of arr
		static bool set(type &arr, int i, ArgIter &it)
		{
			T item;
			bool result = create(item, it);
			if (result) { arr.push_back(item); }
			return result;
		}
		static void from_initializer(type &arr, const std::initializer_list<T> &l)
		{ arr.assign(l.begin(), l.end()); }
	};

	//Print a list-like container.
	template<class T>
	void printvals(std::ostream &o, const T &data)
	{
		o << "[";
		auto it = data.begin();
		if (it != data.end())
		{
			o << *it;
			++it;
			while (it != data.end())
			{
				o << ", " << *it;
				++it;
			}
		}
		o << ']';
	}

	//------------------------------
	//General arg impl
	//------------------------------
	template <class T, int count=1>
	struct TypedArg: public Arg
	{
		typedef MultiDataType<T, count> Typehelp;
		typename Typehelp::type data;
		typedef T rawtype;
		int ndefaults;

		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
			Arg(name, help, reg, true),
			data{},
			ndefaults(0)
		{}

		TypedArg(
			const char *name, const char *help,
			ArgRegistry *reg, std::initializer_list<T> defaults
		):
			Arg(name, help, reg, false),
			data{},
			ndefaults(defaults.size())
		{
			if (ndefaults)
			{
				if (count >= 0 && ndefaults != count)
				{
					throw std::logic_error(
						"The number of defaults should match the count for "
						+ std::string(name));
				}
				Typehelp::from_initializer(data, defaults);
			}
		}
		TypedArg(TypedArg &&other):
			Arg(std::move(static_cast<Arg&>(other))),
			data(other.data),
			ndefaults(other.ndefaults)
		{}

		TypedArg<T, count>& operator=(std::initializer_list<T> vals)
		{
			data = vals;
			return *this;
		}

		//Where in standard says that this
		//would cause something like
		//TypedArg<T,3> x; x[0]
		//to call T* conversion operator? not sure...
		operator T*() { return data.data(); }
		operator const T*() const { return data.data(); }
		decltype(data)& operator*() { return data; }
		const decltype(data)& operator*() const { return data; }
		decltype(data)* operator->() { return &data; }
		const decltype(data)* operator->() const { return &data; }

		void flagspec(std::ostream &o) const override
		{
			o << name;
			if (count < 0) { o << " ..."; }
			else { o << " x" << count; }
		}

		void defaults(std::ostream &o) const override
		{
			if (ndefaults || count < 0)
			{
				o << "(default: ";
				printvals(o, data);
				o << ')';
			}
		}
		int fill(ArgIter &it) override
		{
			Typehelp::clear(data);
			const int num = count > 0 ? count : it.argc;
			int numparsed = 0;
			for (; numparsed<num; ++numparsed)
			{
				if (!it || it.isflag) { break; }
				if (!Typehelp::set(data, numparsed, it))
				{
					if (count > 0) { return 2; }
					break;
				}
			}
			if (count < 0)
			{ if (numparsed == 0 && required) { return 1; } }
			else
			{ if (numparsed != count) { return 1; } }
			return 0;
		}
	};

	//------------------------------
	//Single arg
	//------------------------------
	template <class T>
	struct TypedArg<T, 1>: public Arg
	{
		typedef T rawtype;
		T data;
		bool defaulted;
		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
			Arg(name, help, reg, true),
			data{},
			defaulted(false)
		{}

		TypedArg(
			const char *name, const char *help,
			ArgRegistry *reg, std::initializer_list<T> defaults
		):
			Arg(name, help, reg, false),
			data{},
			defaulted(defaults.size() > 0)
		{
			if (defaulted)
			{
				if (defaults.size() != 1)
				{ throw std::logic_error("Only 1 default value should be given to single-typed arg."); }
				data = *defaults.begin();
			}
		}
		TypedArg(TypedArg &&other):
			Arg(std::move(static_cast<Arg&>(other))),
			data(other.data),
			defaulted(other.defaulted)
		{}

		T& operator*() { return data; }
		const T& operator*() const { return data; }
		T* operator->() { return &data; }
		const T* operator->() const { return &data; }

		TypedArg<T, 1>& operator=(const T &val)
		{
			data = val;
			return *this;
		}

		operator T() { return data; }
		virtual void flagspec(std::ostream &o) const
		{ o << name << ' ' << name; }
		virtual void posspec(std::ostream &o) const
		{ o << name; }
		void defaults(std::ostream &o) const override
		{ if (defaulted) { o << "(default: " << data << ')'; } }

		int fill(ArgIter &it) override
		{
			if (!it || it.isflag) { return 1; }
			return create(data, it) ? 0 : 2;
		}
	};

	//------------------------------
	//counting flags
	//------------------------------
	template<>
	struct TypedArg<bool, 1>: public Arg
	{
		typedef int rawtype;
		int data;
		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
			Arg(name, help, reg, false),
			data(0)
		{}

		TypedArg(
			const char *name, const char *help, ArgRegistry *reg,
			std::initializer_list<int> defaults
		):
			Arg(name, help, reg, false),
			data(defaults.size() > 0 ? *defaults.begin() : 0)
		{
			if (defaults.size() > 1)
			{
				throw std::logic_error(
					"Bool count " + std::string(name) + " should only have 1 initializer.");
			}
		}

		TypedArg(TypedArg &&other):
			Arg(std::move(static_cast<Arg&>(other))),
			data(other.data)
		{}

		TypedArg<bool, 1>& operator=(int val)
		{
			data = val;
			return *this;
		}

		int& operator*() { return data; }
		const int& operator*() const { return data; }

		operator int() { return data; }
		int fill(ArgIter &it) override { ++data; return 0; }
		void flagspec(std::ostream &o) const override { o << name << "++"; }
		void defaults(std::ostream &o) const override {}
	};

	//------------------------------
	//toggle flag
	//------------------------------
	template<>
	struct TypedArg<bool, 0>: public Arg
	{
		typedef bool rawtype;
		bool data;

		TypedArg(const char *name, const char *help, ArgRegistry *reg=nullptr):
			Arg(name, help, reg, false),
			data(false)
		{}

		TypedArg(
			const char *name, const char *help,
			ArgRegistry *reg, std::initializer_list<bool> defaults
		):
			Arg(name, help, reg, false),
			data(defaults.size() ? *defaults.begin() : false)
		{
			if (defaults.size() > 1)
			{
				throw std::logic_error(
				"Bool flag " + std::string(name) + " should only have 1 initializer.");
			}
		}
		TypedArg(TypedArg &&other):
			Arg(std::move(static_cast<Arg&>(other))),
			data(other.data)
		{}

		TypedArg<bool, 0>& operator=(bool val)
		{
			data = val;
			return *this;
		}

		bool& operator*() { return data; }
		const bool& operator*() const { return data; }

		operator bool() { return data; }
		int fill(ArgIter &it) override { data = !data; return 0; }
		void flagspec(std::ostream &o) const override { o << name << "!!"; }
		void defaults(std::ostream &o) const override
		{ o << "(default: " << (data ? "true" : "false") << ')'; }
	};


}
#endif //ARGPARSE_ARG_HPP
