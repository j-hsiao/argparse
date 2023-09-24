#include "argparse/argparse.hpp"
#include "argparse/argiter.hpp"
#include "argstruct.hpp"
#undef NDEBUG
#include <cassert>
#include <string>

int main(int argc, char *argv[])
{
	argparse::Parser p("Test program", "-");


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
