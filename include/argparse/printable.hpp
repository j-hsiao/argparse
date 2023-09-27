//Functions for determining if a type is printable or not
#ifndef ARGPARSE_PRINTABLE_HPP
#define ARGPARSE_PRINTABLE_HPP
#include <ostream>

namespace argparse { namespace check
{
	template<class T, class V=T>
	struct same_type{ static const bool value = false; };
	template<class T>
	struct same_type<T, T>{ static const bool value = true; };

	struct NotPrintable {};

	//use T and V instead of ostream&
	//so lower resolution precedence??
	template<class T, class V>
	NotPrintable operator<<(T &t, const V&)
	{
		t << '?';
		return {};
	}

	template<class T, class V>
	T& print(T &t, const V &v)
	{
		t << v;
		return t;
	}

	template<class T>
	T& lval();

	//print for iterable, generally std::vector/std::array
	template<class T, decltype(lval<T>().begin())*V=nullptr>
	std::ostream& print(std::ostream &o, const T &item)
	{
		auto start = item.begin();
		auto stop = item.end();
		o << "[";
		if (start != stop)
		{
			print(o, *start);
			++start;
		}
		for (; start != stop; ++start)
		{
			o << ", ";
			print(o, *start);
		}
		o << ']';
		return o;
	}

	template<class T>
	struct Printable
	{
		static const bool value = !same_type<
			decltype(lval<std::ostream>() << lval<T>()),
			NotPrintable>::value;
	};
}}
#endif //ARGPARSE_PRINTABLE_HPP
