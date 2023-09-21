#undef NDEBUG
#include <argparse/parse.hpp>
#include <argparse/argiter.hpp>
#include <cassert>
#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
	float f;
	double d;
	int i;
	unsigned int ui;
	short s;
	unsigned short us;
	long L;
	long long LL;
	unsigned long uL;
	unsigned long long uLL;
	const char *c;

	const char * args[] = {
		"-f3.14", "-d1.23", "-i-1", "2", " -1", "3", " -2",
		" -1234", "1234", "12345", "-ohello.txt"
	};
	argparse::ArgIter p(11, args, "-");
	assert(p.isflag == 1);
	assert(p.flag()[0] == 'f');
	p.stepflag();
	bool result = argparse::parse(f, p);
	assert(result);
	assert(f == 3.14f);
	assert(p.isflag == 1);
	assert(p.flag()[0] == 'd');
	p.stepflag();
	result = argparse::parse(d, p);
	assert(result);
	assert(d == 1.23);
	assert(p.isflag == 1);
	assert(p.flag()[0] == 'i');
	p.stepflag();
	result = argparse::parse(i, p);
	assert(result);
	assert(i == -1);
	result = argparse::parse(ui, p);
	assert(result);
	assert(ui == 2);

	result = argparse::parse(s, p);
	assert(result);
	assert(s == -1);

	result = argparse::parse(us, p);
	assert(result);
	assert(us == 3);

	result = argparse::parse(L, p);
	assert(result);
	assert(L == -2);

	result = argparse::parse(LL, p);
	assert(result);
	assert(LL == -1234);

	result = argparse::parse(uL, p);
	assert(result);
	assert(uL == 1234);

	result = argparse::parse(uLL, p);
	assert(result);
	assert(uLL == 12345);

	assert(p.isflag == 1);
	assert(!p.isarg());
	assert(p.flag()[0] == 'o');
	p.stepflag();
	assert(p.isarg());
	result = argparse::parse(c, p);
	assert(result);
	assert(!std::strcmp(c, "hello.txt"));

	return 0;
}
