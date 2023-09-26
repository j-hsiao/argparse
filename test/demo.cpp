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
}

int main(int argc, char *argv[])
{
	using namespace argparse;
	Parser p("test all the args");
	Group required(p, "Required");
	Group optionals(p, "Optional");

	Arg<int> rsnum(required, "required num", "A required integer");
	Arg<short, 3> rnnum(required, "required 3 nums", "3 required short");
	Arg<long, -1> rvnum(required, "required variable nums", "Variable required long");

	Arg<float> osnum(optionals, "optional num", "A optional float", {});
	Arg<double, 3> onnum(optionals, "optional 3 nums", "A optional double", {});
	Arg<const char*, -1> ovnum(optionals, "optional variable nums", "A optional float", {});

	Flag<long long> frsnum(required, "rsnum", "A required long long");
	Flag<short, 3> frnnum(required, "rnnum", "3 required short");
	Flag<long, -1> frvnum(required, "rvnum", "Variable required long");

	Flag<float> fosnum(optionals, "osnum", "A required float", {});
	Flag<double, 3> fonnum(optionals, "onnum", "A required double", {});
	Flag<const char*, -1> fovnum(optionals, "ovnum", "A required float", {});

	Flag<bool, 0> toggle(p, {"t", "toggle"}, "toggle a bool");
	Flag<bool> count(p, {"c", "count"}, "count flag instances");
	Flag<bool> verbose(p, {"v", "verbose"}, "verbosity level", -1);

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
		std::cerr << "Unsuccessful parsing." << std::endl;
		return result.code == result.help ? 0 : 1;
	}

	std::cout << "required num: " << rsnum << std::endl;
	std::cout << "required 3 nums: " << rnnum << std::endl;
	std::cout << "required variable nums: " << rvnum << std::endl;









//	argparse::Parser p;
//	auto verbose = p.add<bool>("-verbose", "verbose, add more for more verbosity.");
//	auto invert = p.add<bool, 0>("-invert", "add to invert.", {true});
//
//	//flag
//	//required
//	auto rscale = p.add<float>("-rscale", "Scaling, required");
//	auto rvec = p.add<float, 2>("-rvec", "vector, required");
//	auto rseq = p.add<float, -1>("-rseq", "sequence, required");
//
//	//not required
//	auto scale = p.add<double>("-scale", "Scaling, not required", {});
//	auto vec = p.add<double, 2>("-vec", "vector, not required.", {1,2});
//	auto seq = p.add<double, -1>("-seq", "sequence, not required", {});
//
//	//pos
//	//required
//	auto rpscale = p.add<int>("rpscale", "Scaling, required");
//	auto rpvec = p.add<int, 2>("rpvec", "vector, required.");
//	auto rpseq = p.add<int, -1>("rpseq", "sequence, required");
//
//	//not required
//	auto pscale = p.add<unsigned int>("pscale", "Scaling, not required", {});
//	auto pvec = p.add<unsigned int, 2>("pvec", "vector, not required.", {1,2});
//	auto pseq = p.add<unsigned int, -1>("pseq", "sequence, not required", {});
//
//	auto dumb = p.add<int>("-dumb", "", {});
//	auto dun = p.add<int>({"-dun", "-duk", "-dur"}, "", {});
//	auto d = p.add<int>("-d", "", {});
//
//	auto cam_args = p.group("camera");
//	auto name = cam_args.add<const char *>("camname", "the camera name.", {"/dev/video0"});
//	auto fps = cam_args.add<float>("-fps", "The frames per second.", {30});
//	auto boolcount = cam_args.add<bool, 1>("-boolcount", "count bools", {5});
//	auto boolr = cam_args.add<bool, 0>("-boolr", "count bools", {true});
//
//	auto result = p.parse(argc, argv);
//
//	if (!result)
//	{
//		if (result.code == 1)
//		{
//			std::cerr << "Exit because help message" << std::endl;
//			return 0;
//		}
//		else
//		{
//			std::cerr << "Exit because parse error" << std::endl;
//			return 1;
//		}
//	}
//
//	std::cerr << "verbose: " << verbose << std::endl;
//	std::cerr << "invert: " << invert << std::endl;
//
//	std::cerr << "rscale: " << rscale << std::endl;
//	std::cerr << "rvec: " << (*rvec)[0] << ',' << (*rvec)[1] << std::endl;
//	std::cerr << "rseq";
//	argparse::printvals(std::cerr, rseq.data);
//	std::cerr << std::endl;
//
//	if (result.parsed("-scale"))
//	{ std::cerr << "scale: " << scale << std::endl; }
//	else
//	{ std::cerr << "-scale was not parsed" << std::endl; }
//	std::cerr << "vec: " << (*vec)[0] << ',' << (*vec)[1] << std::endl;
//	std::cerr << "seq";
//	argparse::printvals(std::cerr, seq.data);
//	std::cerr << std::endl;
//
//	std::cerr << "rpscale: " << rpscale << std::endl;
//	std::cerr << "rpvec: " << (*rpvec)[0] << ',' << (*rpvec)[1] << std::endl;
//	std::cerr << "rpseq";
//	argparse::printvals(std::cerr, rpseq.data);
//	std::cerr << std::endl;
//
//	if (result.parsed("pscale"))
//	{ std::cerr << "pscale: " << pscale << std::endl; }
//	else
//	{ std::cerr << "pscale was not parsed." << std::endl; }
//
//	std::cerr << "pvec: " << (*pvec)[0] << ',' << (*pvec)[1] << std::endl;
//	if (!result.parsed("pvec"))
//	{ std::cerr << "pvec was not parsed." << std::endl; }
//
//	std::cerr << "pseq";
//	argparse::printvals(std::cerr, pseq.data);
//	std::cerr << std::endl;
//	if (!result.parsed("pseq"))
//	{ std::cerr << "pseq was not parsed." << std::endl; }
//
//	std::cerr << "camera: " << name << std::endl;
//	std::cerr << "fps: " << fps << std::endl;
//	std::cerr << "boolcount: " << boolcount << std::endl;
//	std::cerr << "boolr: " << boolr << std::endl;
//
	return 0;
}
