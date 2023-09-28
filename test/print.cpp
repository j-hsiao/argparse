#include "argparse/print.hpp"
#undef NDEBUG
#include <iostream>
#include <ostream>
#include <cassert>
#include <vector>
#include <array>
#include <set>

struct s1
{
};
std::ostream& operator<<(std::ostream &o, const s1& s)
{
	o << "print an s1";
	return o;
}

struct s2
{
};


int main(int argc, char *argv[])
{
	assert(argparse::print::Printable<s1>::value);
	assert(!argparse::print::Printable<s2>::value);
	assert(argparse::print::Printable<int>::value);
	assert(argparse::print::Printable<unsigned int>::value);
	assert(argparse::print::Printable<const char*>::value);
	assert(argparse::print::Printable<short>::value);
	assert(argparse::print::Printable<std::vector<int>>::value);
	assert(argparse::print::Printable<std::vector<unsigned int>>::value);
	assert(argparse::print::Printable<std::vector<const char*>>::value);
	assert(argparse::print::Printable<std::vector<short>>::value);
	assert(argparse::print::Printable<std::vector<s1>>::value);
	assert(!argparse::print::Printable<std::vector<s2>>::value);

	argparse::print::print(std::cout, 69) << std::endl;
	std::vector<int> vec{1, 2, 3, 4};
	argparse::print::print(std::cout, vec) << std::endl;

	std::vector<std::array<int, 5>> vecarr{
		{1, 2, 3, 4, 5},
		{5, 4, 3, 2, 1}};
	argparse::print::print(std::cout, vecarr) << std::endl;

	std::set<std::string> s{"hello", "world"};
	argparse::print::print(std::cout, s) << std::endl;

	argparse::print::print(std::cout, s1{}) << std::endl;
	argparse::print::print(std::cout, s2{}) << std::endl;

	std::vector<s1> s1s{{}, {}, {}};
	std::vector<s2> s2s{{}, {}, {}};

	argparse::print::print(std::cout, s1s) << std::endl;
	argparse::print::print(std::cout, s2s) << std::endl;

	return 0;
}
