#undef NDEBUG

#include "argparse/argparse.hpp"
#include "argparse/argiter.hpp"
#include "argstruct.hpp"
#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
	argparse::Parser p("Test program", "-");
	using namespace argparse;
	Arg<int> num(p, "num", "A number, required");
	assert(num.required);

	Group grp1(p, "geometry");
	Flag<float, 3> vec(grp1, {"vec", "v"}, "Vector x y z", {});
	Flag<float, 3> point(grp1, {"point", "p"}, "Point x y z", {1, 2, 3});
	Flag<bool, 0> invert(grp1, "i", "Invert axes.");
	assert(!vec.required);
	assert(!invert.required);
	assert(!invert);

	{
		const char* args[] = {
			"-iv1", "2", "3", "--point", "5", "6", "--1", "-3",
			" -42"
		};
		auto result = p.parse(args, argv[0]);
		assert(result.code == 0);
		assert(invert);
		assert(vec[0] == 1);
		assert(vec[1] == 2);
		assert(vec[2] == 3);
		assert(point[0] == 5);
		assert(point[1] == 6);
		assert(point[2] == -3);
		assert(*num == -42);
		assert(result.parsed(vec));
		assert(result.parsed(point));
		assert(result.parsed(invert));
		assert(result.parsed(num));
	}

	{
		const char* args[] = {"3", "2"};
		auto result = p.parse(args, argv[0]);
		assert(result.code == 2);
	}



//	{
//		argparse::Parser p;
//
//		auto xyz = p.add<int, 3>("xyz", "point coordinates");
//		assert(p.pos.size() == 1);
//		assert(p.pos[0] == &xyz);
//		assert(xyz.required);
//
//		auto fxyz = p.add<int, 3>("--xyz", "point coordinates", {1,2,3});
//		assert(p.pos.size() == 1);
//		assert(p.flags.size() == 1);
//		assert(p.pos[0] == &xyz);
//		assert(p.flags[0] == &fxyz);
//		assert(fxyz.name == std::string("xyz"));
//		assert(!fxyz.required);
//	}
//
//	{
//		argparse::Parser p;
//		auto hen = p.add<const char*>("-hen", "name of a hen", {"Henrietta"});
//		auto hez = p.add<const char*>("-hez", "some flag");
//		auto pos1 = p.add<const char*>("pos1", "some required positional argument");
//		auto pos2 = p.add<const char*>("pos2", "some optional positional argument", {"some default"});
//		{
//			auto args = ::args("arg1", "whatever", "-someflag", "--someflag", "-he");
//			argparse::ArgIter it(args.size(), args.args);
//			assert(p.findhelp(it) == 0);
//		}
//		{
//			auto args = ::args("arg1", "whatever", "-someflag", "--someflag", "-hel");
//			argparse::ArgIter it(args.size(), args.args);
//			assert(p.findhelp(it) == 1);
//		}
//		{
//			auto args = ::args("arg1", "whatever", "-someflag", "--someflag", "-help");
//			argparse::ArgIter it(args.size(), args.args);
//			assert(p.findhelp(it) == 2);
//		}
//		{
//			auto args = ::args("arg1", "whatever", "-someflag", "--someflag", "--h");
//			argparse::ArgIter it(args.size(), args.args);
//			assert(p.findhelp(it) == 1);
//		}
//		{
//			auto args = ::args("arg1", "whatever", "-someflag", "--someflag", "--help");
//			argparse::ArgIter it(args.size(), args.args);
//			assert(p.findhelp(it) == 2);
//		}
//
//		p.dohelp("programname", 1);
//		p.dohelp("programname", 2);
//	}
//
//	{
//		argparse::Parser p;
//		auto cam = p.group("camera");
//		auto camname = cam.add<const char*>("name", "name of the camera source.");
//		auto fps = cam.add<float>("-fps", "frames per second", {25});
//		auto resolution = cam.add<int, 2>({"-shape", "-s"}, "shape of frame, width height", {1920, 1080});
//
//		p.dohelp("prog", 1);
//		p.dohelp("prog", 2);
//
//	}
	return 0;
}
