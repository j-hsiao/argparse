#ifndef ARGSTRUCT_HPP
#define ARGSTRUCT_HPP
namespace
{
	template<std::size_t count>
	struct Args
	{
		std::size_t size() const { return count; }
		const char *args[count];

		const char* operator[](std::size_t idx) const { return args[idx]; }
		void fill(std::size_t idx) {}
		template<class...V>
		void fill(std::size_t idx, const char *arg, V...v)
		{
			args[idx] = arg;
			fill(idx+1, v...);
		}
	};

	template<class...T>
	static Args<sizeof...(T)> args(T...strs)
	{
		Args<sizeof...(T)> ret;
		ret.fill(0, strs...);
		return ret;
	}

}
#endif
