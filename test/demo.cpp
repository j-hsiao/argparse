// required?
// flag or pos?
// 1, multi, var, ...
//
#include <argparse/argparse.hpp>

int main(int argc, char *argv[])
{
	argparse::Parser p;
	auto verbose = p.add<bool>("-verbose", "verbose, add more for more verbosity.");
	auto invert = p.add<bool, 0>("-invert", "add to invert.", {true});

	//flag
	//required
	auto rscale = p.add<float>("-rscale", "Scaling, required");
	auto rvec = p.add<float, 2>("-rvec", "vector, required");
	auto rseq = p.add<float, -1>("-rseq", "sequence, required");

	//not required
	auto scale = p.add<double>("-scale", "Scaling, not required", {});
	auto vec = p.add<double, 2>("-vec", "vector, not required.", {1,2});
	auto seq = p.add<double, -1>("-seq", "sequence, not required", {});

	//pos
	//required
	auto rpscale = p.add<int>("rpscale", "Scaling, required");
	auto rpvec = p.add<int, 2>("rpvec", "vector, required.");
	auto rpseq = p.add<int, -1>("rpseq", "sequence, required");

	//not required
	auto pscale = p.add<unsigned int>("pscale", "Scaling, not required", {});
	auto pvec = p.add<unsigned int, 2>("pvec", "vector, not required.", {1,2});
	auto pseq = p.add<unsigned int, -1>("pseq", "sequence, not required", {});

	auto dumb = p.add<int>("-dumb", "", {});
	auto dun = p.add<int>({"-dun", "-duk", "-dur"}, "", {});
	auto d = p.add<int>("-d", "", {});

	auto cam_args = p.group("camera");
	auto name = cam_args.add<const char *>("camname", "the camera name.", {"/dev/video0"});
	auto fps = cam_args.add<float>("-fps", "The frames per second.", {30});
	auto boolcount = cam_args.add<bool, 1>("-boolcount", "count bools", {5});
	auto boolr = cam_args.add<bool, 0>("-boolr", "count bools", {true});

	auto result = p.parse(argc, argv);

	if (!result)
	{
		if (result.code == 1)
		{
			std::cerr << "Exit because help message" << std::endl;
			return 0;
		}
		else
		{
			std::cerr << "Exit because parse error" << std::endl;
			return 1;
		}
	}

	std::cerr << "verbose: " << verbose << std::endl;
	std::cerr << "invert: " << invert << std::endl;

	std::cerr << "rscale: " << rscale << std::endl;
	std::cerr << "rvec: " << (*rvec)[0] << ',' << (*rvec)[1] << std::endl;
	std::cerr << "rseq";
	argparse::printvals(std::cerr, rseq.data);
	std::cerr << std::endl;

	if (result.parsed("-scale"))
	{ std::cerr << "scale: " << scale << std::endl; }
	else
	{ std::cerr << "-scale was not parsed" << std::endl; }
	std::cerr << "vec: " << (*vec)[0] << ',' << (*vec)[1] << std::endl;
	std::cerr << "seq";
	argparse::printvals(std::cerr, seq.data);
	std::cerr << std::endl;

	std::cerr << "rpscale: " << rpscale << std::endl;
	std::cerr << "rpvec: " << (*rpvec)[0] << ',' << (*rpvec)[1] << std::endl;
	std::cerr << "rpseq";
	argparse::printvals(std::cerr, rpseq.data);
	std::cerr << std::endl;

	if (result.parsed("pscale"))
	{ std::cerr << "pscale: " << pscale << std::endl; }
	else
	{ std::cerr << "pscale was not parsed." << std::endl; }

	std::cerr << "pvec: " << (*pvec)[0] << ',' << (*pvec)[1] << std::endl;
	if (!result.parsed("pvec"))
	{ std::cerr << "pvec was not parsed." << std::endl; }

	std::cerr << "pseq";
	argparse::printvals(std::cerr, pseq.data);
	std::cerr << std::endl;
	if (!result.parsed("pseq"))
	{ std::cerr << "pseq was not parsed." << std::endl; }

	std::cerr << "camera: " << name << std::endl;
	std::cerr << "fps: " << fps << std::endl;
	std::cerr << "boolcount: " << boolcount << std::endl;
	std::cerr << "boolr: " << boolr << std::endl;

	return 0;
}
