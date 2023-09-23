//Functions for determining if a type is printable or not

#include <ostream>

namespace argparse { namespace check
{
	template<class T, class V=T>
	struct same_type{ static const bool value = false; };
	template<class T>
	struct same_type<T, T>{ static const bool value = true; };

	template<class T>
	T inst();

	struct NotPrintable {};

	//use T and V instead of ostream&
	//so lower resolution precedence??
	template<class T, class V>
	NotPrintable operator<<(T &t, const V&)
	{
		t << '?';
		return {};
	}

	template<class T>
	struct Printable
	{
		static const bool value = !same_type<
			decltype(*static_cast<std::ostream*>(nullptr) << inst<T>()),
			NotPrintable>::value;
	};
}}
