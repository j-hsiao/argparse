#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
	{
		using namespace argparse;
		Parser p1("test using argparse");
		Flag<const char*> prefix(p1, "prefix", "The prefix char.", "-");
		Arg<const char*, -1> remainder(p1, "args", "args for secondary parser", {});
		ParseResult result1 = p1.parse(argc, argv);
		if (result1)
		{ return result1.code == result1.help ? 0 : 1; }

		Parser p2("Test with custom prefix", *prefix);
		Arg<int> num(p2, "num", "some number", {});
		Flag<bool> f(p2, "f", "some bool toggle flag");
		ParseResult result2 = p2.parse(remainder->size(), &remainder[0], argv[0]);

		if (result2)
		{ return result2.code == result2.help ? 0 : 1; }
		if (result2.parsed(num))
		{ std::cerr << "num: " << num << std::endl; }
		std::cerr << "f: " << static_cast<int>(f) << std::endl;
	}
	return 0;
}
