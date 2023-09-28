// Check if an item is printable and printing functions
#ifndef ARGPARSE_PRINTABLE_HPP
#define ARGPARSE_PRINTABLE_HPP
#include <ostream>

namespace argparse { namespace print
{
	template<class T, class V=T>
	struct same_type{ static const bool value = false; };
	template<class T>
	struct same_type<T, T>{ static const bool value = true; };
	template<class T>
	struct enabled { static const bool value = true; };

	template<bool v>
	struct enabled_if;
	template<>
	struct enabled_if<true> {};

	template<class T>
	T& lval();

	//Both template arguments so lowest precedence
	//compared to std::ostream
	template<class T, class V>
	void operator<<(T &t, const V &v) { t << '?'; }

	template<class T, class V, bool s=true>
	struct Print
	{
		void operator()(T &o, const V &v) const
		{ o << v; }
	};

	template<class T, class V>
	T& print(T &t, const V &v) { Print<T, V>{}(t, v); return t; }

	template<class T, class V>
	struct Print<
		T, V,
		enabled<decltype(lval<V>().begin())>::value
			&& enabled<typename V::value_type>::value
			&& same_type<decltype(lval<std::ostream>() << lval<V>()), void>::value
			>
	{
		void operator()(T &o, const V &v) const
		{
			auto start = v.begin();
			auto stop = v.end();
			o << '[';
			if (start != stop)
			{
				print(o, *start);
				++start;
			}
			for (; start != stop; ++start)
			{ print(o << ", ", *start); }
			o << ']';
		}
	};

	template<class T, bool S=true>
	struct Printable
	{ static const bool value = !same_type<decltype(lval<std::ostream>() << lval<T>()), void>::value; };

	template<class T>
	struct Printable<
		T,
		enabled<typename T::value_type>::value
			&& enabled<decltype(lval<T>().begin())>::value
		>: public Printable<typename T::value_type>
	{};
}}
#endif //ARGPARSE_PRINTABLE_HPP
