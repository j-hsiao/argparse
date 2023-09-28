#ifndef ARGPARSE_ARGITER_HPP
#define ARGPARSE_ARGITER_HPP
#include <cstddef>

namespace argparse
{
	struct ArgIter
	{
		//> 0 if isflag else not a flag (doesn't start with prefix)
		std::size_t isflag;
		const char *arg;
		private:
			int argc, pos;
			const char * const *argv;
			const char *prefix;
			int forcepos;
		public:

		template<class T, int N>
		ArgIter(T (&args)[N], const char *prefix="-"):
			ArgIter(N, args, prefix)
		{}

		ArgIter(int argc, const char * const argv[], const char *prefix="-");

		operator bool() const { return pos < argc; }
		void finish() { pos = argc; }
		void reset();
		bool isarg() const;
		bool breakpoint() const;
		//name of flag without prefix chars

		void stepbreak();

		//Assume the current arg is a short flag (isflag == 1).
		//Step through the current short-flag position
		//If the current arg is exhausted, then step
		//to the next arg.
		void stepflag();

		//step to the next arg.
		void step();
	};
}
#endif //ARGPARSE_ARGITER_HPP
