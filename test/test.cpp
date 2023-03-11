#include "argparse/argparse.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

template<std::size_t N>
struct charp
{
	std::vector<char*> args;
	charp(char (*argv)[N], std::size_t size)
	{
		for (std::size_t i=0; i<size / N; ++i)
		{ args.push_back(argv[i]); }
	}

	char** argv() { return &args[0]; }
	int argc() { return static_cast<int>(args.size()); }
};

int main(int argc, char *argv[])
{
#	define pre "-"
	{
		argparse::Parser p("test argument parsing", pre);
		// flags test
		// single
		float &ffval = p.add<float>(pre "ffval", "float flag value");
		int &ifval = p.add<int>(pre "ifval", "int flag value", {});

		//fixed
		auto &ffvalf = p.add<float, 2>(pre "ffvalf", "float flag fixed");
		auto &ifvalf = p.add<int, 2>(pre "ifvalf", "int flag fixed");

		//multi
		auto &ffvalm = p.add<float, -1>(pre "ffvalm", "float flag multi");
		auto &ifvalm = p.add<int, -1>(pre "ifvalm", "int flag multi");

		//single bools
		auto &bfvalt = p.add<bool, 0>(pre "bfvalt", "bool flag toggle");
		auto &bfvalc = p.add<bool, 1>(pre "bfvalc", "bool flag count");

		// positional test
		// single
		float &fpval = p.add<float>("fpval", "float positional value");
		int &ipval = p.add<int>("ipval", "int positional value", {});

		//fixed
		auto &fpvalf = p.add<float, 2>("fpvalf", "float positional fixed");
		auto &ipvalf = p.add<int, 2>("ipvalf", "int positional fixed");

		//multi
		auto &fpvalm = p.add<float, -1>("fpvalm", "float positional multi");
		auto &ipvalm = p.add<int, -1>("ipvalm", "int positional multi");

		if (argc < 2)
		{
			char hargs[] = pre "help";
			char *hargp = hargs;
			hargs[2] = '\0';
			assert(!p.parse(1, &hargp, hargp));
			hargs[2] = 'e';
			assert(!p.parse(1, &hargp, hargp));

			char rawargs[][10] = {
				"prog", pre "ffval", "2.5", pre "ifval", " -32", pre "ffvalf",
				pre pre "1", "-0.5", "0.125", pre "ifvalf", "1", "2", pre "ffvalm",
				"0.75", "0.5", "0.25", pre "ifvalm", "52", "69", "42", "15",
				pre "bfvalt", pre "bfvalc", pre"bfvalc", pre"bfvalc",
				"25.5", "73", "69.5", "69.25", "1024", "4096", "1.25", "2.25", "3.25",
				pre "bfvalt", pre "bfvalt", "1", "2", "3", "4"
			};
			charp args(rawargs, sizeof(rawargs));
			assert(p.parse(args.argc(), args.argv()));
			assert(ffval == 2.5f);
			assert(ifval == -32);
			assert(ffvalf.size() == 2);
			assert(ffvalf[0] == -0.5f);
			assert(ffvalf[1] == 0.125f);
			assert(ifvalf.size() == 2);
			assert(ifvalf[0] == 1);
			assert(ifvalf[1] == 2);
			assert(ffvalm.size() == 3);
			assert(ffvalm[0] == 0.75f);
			assert(ffvalm[1] == 0.5f);
			assert(ffvalm[2] == 0.25f);
			assert(ifvalm[0] == 52);
			assert(ifvalm[1] == 69);
			assert(ifvalm[2] == 42);
			assert(ifvalm[3] == 15);
			assert(bfvalt);
			assert(bfvalc == 3);
			assert(fpval == 25.5f);
			assert(ipval == 73);
			assert(fpvalf[0] == 69.5);
			assert(fpvalf[1] == 69.25);
			assert(ipvalf[0] == 1024);
			assert(ipvalf[1] == 4096);
			assert(fpvalm.size() == 3);
			assert(fpvalm[0] == 1.25f);
			assert(fpvalm[1] == 2.25f);
			assert(fpvalm[2] == 3.25f);
			assert(ipvalm.size() == 4);
			for (int i=0; i<ipvalm.size(); ++i)
			{ assert(ipvalm[i] == i+1); }
		}
		else
		{ p.parse(argc, argv); }
	}
#define SETUP \
	argparse::Parser p("test argument parsing", pre); \
	bool &ttoggled = p.add<bool, 0>("-ttoggled", "", true); \
	bool &ftoggled = p.add<bool, 0>("-ftoggled", "", false); \
	char t[] = "-t"; \
	char f[] = "-f"
	{
		SETUP;
		std::vector<char*> args = {t, t, t, f, f, f};
		assert(p.parse(args.size(), &args[0], argv[0]));
		assert(!ttoggled && ftoggled);
	}
	{
		SETUP;
		std::vector<char*> args = {t, f, t, f};
		assert(p.parse(args.size(), &args[0], argv[0]));
		assert(ttoggled && !ftoggled);
	}
	{
		argparse::Parser p("", pre);
		auto &initial = p.add<std::string>("initial");
		auto &remainder = p.add<char*, argparse::Remainder>("remain");

		char buf[][20] = {"initial value", pre pre, pre "help"};
		char *args[] = {buf[0], buf[1], buf[2]};

		assert(p.parse(3, args, buf[0]));
		assert(initial == "initial value");
		assert(remainder.argc == 1);
		assert(remainder.argv[0] == std::string(pre "help"));
	}
	{
		argparse::Parser p("", pre);
		auto &b = p.add<bool>("-bool");
		auto &i = p.add<int>("num");
		char buf[][20] = {"prog", pre pre "1", "-69", "-b"};
		charp args(buf, sizeof(buf));

		assert(p.parse(args.argc(), args.argv()));
		assert(b);
		assert(i==-69);
	}
	{
		{
			argparse::Parser p("", pre);
			auto &h1 = p.add<int>("--help", "full", {69});
			auto &h2 = p.add<int>("-helper", "partial", {70});
			char bufs[][20] = {"-help", "20"};
			charp args(bufs, sizeof(bufs));
			assert(p.parse(args.argc(), args.argv(), ""));
			assert(h1 == 20 && h2 == 70);
		}
		{
			argparse::Parser p("", pre);
			auto &h1 = p.add<int>("--help", "full", {69});
			auto &h2 = p.add<int>("-helper", "partial", {70});
			char bufs[][20] = {"--help", "20"};
			charp args(bufs, sizeof(bufs));
			assert(!p.parse(args.argc(), args.argv(), ""));
		}
		{
			argparse::Parser p("", pre);
			auto &h1 = p.add<int>("--help", "full", {69});
			auto &h2 = p.add<int>("-helper", "partial", {70});
			char bufs[][20] = {"-he", "20"};
			charp args(bufs, sizeof(bufs));
			assert(p.parse(args.argc(), args.argv(), ""));
			assert(h1 == 69 && h2 == 20);
		}
	}
	return 0;
}
