//test argparse2 interface
#include "argparse/argparse.hpp"

#include <iostream>


int main(int argc, char *argv[])
{
	argparse::Parser p;
	auto &shape = p.add<int, 2>({"-shape", "-s"}, "width, height");
	auto &fps = p.add<float>("fps", "the fps", 30.0f);
	auto &something = p.add<bool, 0>("-something");
	auto &rest = p.add<char*, -1>("remaining");
	auto &flagcount = p.add<int, 0>("-flagcount", "count number of currences of this flag");
	if (p.parse_main(argc, argv))
	{ return 1; }
	std::cout << shape[0] << " x " << shape[1] << " @ " << fps << " fps" << std::endl;
	std::cout << "do something? " << something << std::endl;
	std::cout << "-flagcount appeared " << flagcount << " times." << std::endl;
	std::cout << "remaining args:" << std::endl;
	for (const char *arg: rest)
	{ std::cout << '\t' << arg << std::endl; }
	return 0;
}
