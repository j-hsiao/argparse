#include "argparse/argparse.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
#	define pre "+"
	argparse::Parser p("test argument parsing", pre);
	auto &shape = p.add<int,-1>(pre "shape", "a shape, width height", {1920, 1080});
	if (p.parse(argc, argv))
	{ return 1; }


	std::cout << "shape: " << shape[0] << ", " << shape[1] << std::endl;

	return 0;
}
