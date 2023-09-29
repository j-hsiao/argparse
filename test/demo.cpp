// required?
// flag or pos?
// 1, multi, var, ...
//
#include <argparse/argparse.hpp>
#include <iostream>

namespace mynamespace
{
	struct Point { int x, y, z; };
	bool parse(Point &p, argparse::ArgIter &it)
	{ return parse(p.x, it) && parse(p.y, it) && parse(p.z, it); }

	std::ostream& operator<<(std::ostream &o, const Point &p)
	{
		o << '<' << p.x << ", " << p.y << ", " << p.z << '>';
		return o;
	}
}

namespace
{
	template<class T>
	void display(const char *desc, argparse::ParseResult &result, const T &t)
	{
		std::cout << desc << ": ";
		if (result.parsed(t))
		{ std::cout << t << std::endl; }
		else
		{ std::cout << "Not parsed" << std::endl; }
	}
}

int main(int argc, char *argv[])
{
	try
	{
		using namespace argparse;
		Parser p("test all the args");
		Group required(p, "Required");
		Group optionals(p, "Optional");

		Arg<int> rsnum(required, "required num", "A required integer");
		Arg<short, 3> rnnum(required, "required 3 nums", "3 required shorts");
		Arg<long, -1> rvnum(required, "required variable nums", "Variable required longs");

		Arg<float> osnum(optionals, "optional num", "A optional float", {});
		Arg<double, 3> onnum(optionals, "optional 3 nums", "3 optional doubles", {});
		Arg<const char*, -1> ovchar(optionals, "optional variable strings", "variable optional strings", {});

		Flag<long long> frsnum(required, "rsnum", "A required long long flag");
		Flag<short, 3> frnnum(required, "rnnum", "3 required shorts flag");
		Flag<long, -1> frvnum(required, "rvnum", "Variable required longs flag");

		Flag<float> fosnum(optionals, "osnum", "A optional float flag", {});
		Flag<double, 3> fonnum(optionals, "onnum", "3 optional doubles", {});
		Flag<const char*, -1> fovchar(optionals, "ovchar", "optional variable strings", {});

		Flag<bool, 0> toggle(p, {"t", "toggle"}, "toggle a bool");
		Flag<bool> count(p, {"c", "count"}, "count flag instances");
		Flag<bool> verbose(p, {"v", "verbose"}, "verbosity level", -1);

		Flag<std::vector<const char*>, -1> sentences(p, {"s", "sentences"}, "variable number of sentences.", {{"default", "sentence."}});

		Flag<Base<int, 16>, 1> hex(p, "hex", "A hex number.", {});

		Arg<mynamespace::Point> rspoint(
			p, "required num",
			"Actually a point, but clashing names for arguments are allowed"
			" because they are unambiguous.  Positional arguments are determined"
			" by position so name does not really matter.  However, the help"
			" message might be a little confusing so additionally display index"
			" for any args with clashing names.", {});

		auto result = p.parse(argc, argv);
		if (result)
		{
			if (result.code == result.help)
			{
				std::cout << "Help message activated." << std::endl;
				return 0;
			}
			else
			{
				std::cerr << "Failed parsing." << std::endl;
				return 1;
			}
		}

		std::cout << "required num: " << rsnum << std::endl;
		std::cout << "required 3 nums: " << rnnum << std::endl;
		std::cout << "required variable nums: " << rvnum << std::endl;

		std::cout << "required num flag: " << frsnum << std::endl;
		std::cout << "required 3 num flag: " << frnnum << std::endl;
		std::cout << "required variable nums flag: " << frvnum << std::endl;

		display("optional num", result, osnum);
		display("optional 3 num", result, onnum);
		display("optional variable strings", result, ovchar);

		display("optional num flag", result, fosnum);
		display("optional 3 num flag", result, fonnum);
		display("optional variable strings flag", result, fovchar);

		std::cout << "toggle flag: " << (toggle ? "true" : "false") << std::endl;
		std::cout << "count: " << count << std::endl;
		std::cout << "verbose: " << verbose << std::endl;

		std::cout << "point: " << rspoint << std::endl;

		std::cout << "number of sentences: " << sentences->size() << std::endl;
		if (sentences->size())
		{
			std::cout << "sentences:" << std::endl;
			for (auto &sentence : *sentences)
			{
				std::cout << "  ";
				argparse::print::print(std::cout, sentence) << std::endl;
			}
		}

		//hex is Flag<Base<int, 16>, 1>
		//The first * accesses the Base<int,16> inside hex
		//The second * accesses the int inside hex.data
		if (result.parsed(hex))
		{ std::cerr << "hex was parsed to " << **hex << std::endl; }

	}
	catch (std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
