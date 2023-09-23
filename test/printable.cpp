#include "argparse/printable.hpp"
#undef NDEBUG
#include <ostream>
#include <cassert>

struct s1
{
};
std::ostream& operator<<(std::ostream &o, const s1& s)
{
	o << "print an s1" << std::endl;
	return o;
}

struct s2
{
};


int main(int argc, char *argv[])
{
	assert(argparse::check::Printable<s1>::value);
	assert(!argparse::check::Printable<s2>::value);
	assert(argparse::check::Printable<int>::value);
	assert(argparse::check::Printable<unsigned int>::value);
	assert(argparse::check::Printable<const char*>::value);
	assert(argparse::check::Printable<short>::value);

	return 0;
}
