#include "argparse/nums.hpp"

#include <cstring>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace argparse
{
#define SPECIALIZE(T, valcheck, prefix) \
	template<> \
	bool store<prefix T>(prefix T &dst, const char *arg, int base) \
	{ \
		prefix long long v; \
		if (store(v, arg, base) && valcheck) \
		{ \
			dst = static_cast<prefix T>(v); \
			return true; \
		} \
		return false; \
	}
	SPECIALIZE(int, INT_MIN <= v && v <= INT_MAX, )
	SPECIALIZE(short, SHRT_MIN <= v && v <= SHRT_MAX, )
	SPECIALIZE(short, v <= USHRT_MAX, unsigned)
	SPECIALIZE(int, v <= UINT_MAX, unsigned)
#undef SPECIALIZE
}

#include "argparse/argparse.hpp"
namespace argparse
{
	namespace
	{
		template<class T>
		const char* most(const std::vector<const char*> &names, T op={})
		{
			auto it = names.begin();
			auto length = std::strlen(*it);
			const char *nm = *it;
			for (; it != names.end(); ++it)
			{
				std::size_t nl = std::strlen(*it);
				if (op(nl, length)) { nm = *it; length = nl; }
			}
			return nm;
		}
		struct lt { bool operator()(std::size_t a, std::size_t b) { return a < b; } };
		struct gt { bool operator()(std::size_t a, std::size_t b) { return a > b; } };


		struct Flagname
		{
			const char *prefix;
			const char *name;
		};

		std::ostream& operator<<(std::ostream &o, const Flagname &f)
		{
			o << f.prefix;
			if (f.name[1]) { o << f.prefix; }
			o << f.name;
			return o;
		}

		std::string& operator+=(std::string &s, const Flagname &f)
		{
			s += f.prefix;
			if (f.name[1]) { s += f.prefix; }
			s += f.name;
			return s;
		}

		struct ArgCount { const ArgCommon *arg; };
		struct FlagCount { const ArgCommon *arg; };
		struct ArgDefaults { const ArgCommon *arg; };

		std::ostream& operator<<(std::ostream &o, const ArgCount &c)
		{
			c.arg->print_acount(o);
			return o;
		}
		std::ostream& operator<<(std::ostream &o, const FlagCount &c)
		{
			c.arg->print_count(o);
			return o;
		}

		std::ostream& operator<<(std::ostream &o, const ArgDefaults &c)
		{
			c.arg->print_defaults(o);
			return o;
		}
	}

	Parser::Parser(
		const char *description, const char *prefix,
		std::ostream &out
	):
		description(description),
		prefix(prefix),
		out(out)
	{}

	void Parser::add(ArgCommon &arg)
	{
		if (arg.names.size() > 1)
		{ throw std::logic_error("Positional arg should have only 1 name."); }
		if (pos.size() && arg.required && !pos.back()->required)
		{
			throw std::logic_error(
				"Required positional arg after optional positional arg.");
		}
		pos.push_back(&arg);
	}

	void Parser::add(FlagCommon &arg)
	{
		for (const char *name : arg.names)
		{
			auto result = flags.insert({name, &arg});
			if (!result.second)
			{ throw std::logic_error("Flag already added."); }
		}
	}

	ParseResult Parser::parse(ArgIter &it, const char *program) const
	{
		if (prehelp(it, program)) { return {ParseResult::help, {}, this}; }
		ParseResult result{ParseResult::success, {}, this};
		auto posit = pos.begin();
		while (it)
		{
			if (it.isflag == 1)
			{ if (handle_shortflag(it, result, program)) { return result; } }
			else if (it.isflag && !it.breakpoint())
			{ if (handle_longflag(it, result)) { return result; } }
			else
			{ if (handle_positional(it, result, posit)) { return result; } }
		}
		check_required(result, posit);
		return result;
	}

	bool Parser::prehelp(ArgIter &it, const char *program) const
	{
		while (it)
		{
			if (it.isflag == 2 && !std::strcmp(it.arg, "help") && flags.find("help") == flags.end())
			{
				do_fullhelp(program);
				return true;
			}
			else if (it.isflag == 1 && !std::strcmp(it.arg, "h") && flags.find("h") == flags.end())
			{
				do_shorthelp(program);
				return true;
			}
			it.step();
		}
		it.reset();
		return false;
	}

	void Parser::do_shorthelp(const char *program) const
	{
		const char *wrap[] = {"[]", "<>"};
		std::set<const FlagCommon*> handled;
		out << "Usage: " << program;
		for (auto flagpair : flags)
		{
			auto flagpt = flagpair.second;
			if (handled.find(flagpt) != handled.end())
			{ continue; }
			handled.insert(flagpt);
			out << ' ' << wrap[flagpt->required][0]
				<< Flagname{prefix, most<lt>(flagpt->names)}
				<< FlagCount{flagpt} << wrap[flagpt->required][1];
		}
		for (auto argpt : pos)
		{
			out << ' ' << wrap[argpt->required][0] << argpt->names[0]
				<< ArgCount{argpt} << wrap[argpt->required][1];
		}
		out << std::endl;
	}

	void Parser::do_fullhelp(const char *program) const
	{
		std::map<const char *, int, Cmp> namecount;
		for (auto argpt: pos)
		{
			auto result = namecount.insert({argpt->names[0], 1});
			if (!result.second)
			{
				result.first->second += 1;
				out << "repeated arg: " << argpt->names[0] << std::endl;
			}
		}

		do_shorthelp(program);
		if (description)
		{ out << std::endl << description << std::endl; }
		std::set<ArgCommon*> handled;
		for (Group *group: groups)
		{
			std::vector<ArgCommon*> groupflags;
			std::vector<ArgCommon*> grouppos;
			for (ArgCommon *arg : group->members)
			{
				auto it = flags.find(arg->names[0]);
				if (it == flags.end() || it->second != arg)
				{ grouppos.push_back(arg); }
				else
				{ groupflags.push_back(arg); }
				handled.insert(arg);
			}
			out << std::endl << group->name << " args:" << std::endl;
			if (groupflags.size()) { out << "  Flags:" << std::endl; }
			for (auto &flag : groupflags) { flaghelp(flag, "    "); }
			if (grouppos.size()) { out << "  Positional Arguments:" << std::endl; }
			for (auto &arg : grouppos) { arghelp(arg, "    ", namecount); }
		}
		bool header = true;
		for (auto pair: flags)
		{
			if (handled.find(pair.second) == handled.end())
			{
				if (header)
				{
					out << std::endl << "Flags:" << std::endl;
					header = false;
				}
				flaghelp(pair.second, "  ");
			}
		}
		header = true;
		for (auto argpt: pos)
		{
			if (handled.find(argpt) == handled.end())
			{
				if (header)
				{
					out << std::endl << "Positional Arguments:" << std::endl;
					header = false;
				}
				arghelp(argpt, "  ", namecount);
			}
		}
	}

	void Parser::flaghelp(const ArgCommon *flag, const char *indent) const
	{
		const char *wrap[] = {"[]", "<>"};
		auto nameit = flag->names.begin();
		out << indent << wrap[flag->required][0]
			<< Flagname{prefix, *nameit++};
		for (; nameit != flag->names.end(); ++nameit)
		{ out << " | " << Flagname{prefix, *nameit}; }
		out << wrap[flag->required][1] << ArgDefaults{flag} << std::endl;
		if (flag->help)
		{ out << indent << "  " << flag->help << std::endl; }
	}

	void Parser::arghelp(
		const ArgCommon *arg, const char *indent,
		const std::map<const char *, int, Cmp> &namecount) const
	{
		const char *wrap[] = {"[]", "<>"};
		out << indent << wrap[arg->required][0] << arg->names[0]
			<< wrap[arg->required][1];
		if (namecount.at(arg->names[0]) > 1)
		{
			for (std::size_t i=0; i<pos.size(); ++i)
			{
				if (pos[i] == arg)
				{
					out << " (Positional index " << i << ')';
					i = pos.size();
				}
			}
		}
		out << ArgDefaults{arg} << std::endl;
		if (arg->help) { out << indent << "  " << arg->help << std::endl; }
	}

	int Parser::handle_shortflag(
		ArgIter &it, ParseResult &result,
		const char *program) const
	{
		char check[2] = {it.arg[0], '\0'};
		auto flag = flags.find(check);
		if (flag == flags.end())
		{
			if (check[0] == 'h' && !it.arg[1])
			{
				do_shorthelp(program);
				return result.code = result.help;
			}
			out << "Unknown flag \"" << prefix <<
				check[0] << '"' << std::endl;
			return result.code = result.unknown;
		}
		it.stepflag();
		if (!flag->second->parse(it))
		{
			out << "Error parsing flag \"" << prefix
				<< check[0] << '"' << std::endl;
			return result.code = result.error;
		}
		result.args.insert(flag->second);
		return 0;
	}

	int Parser::handle_longflag(ArgIter &it, ParseResult &result) const
	{
		const char *name = it.arg;
		auto flag = flags.find(name);
		if (flag == flags.end())
		{
			out << "Unknown flag \"" << prefix << prefix
				<< name << '"' << std::endl;
			return result.code = result.unknown;
		}
		it.step();
		if (!flag->second->parse(it))
		{
			out << "Error parsing flag \"" << prefix << prefix
				<< name << '"' << std::endl;
			return result.code = result.error;
		}
		result.args.insert(flag->second);
		return 0;
	}

	int Parser::handle_positional(
		ArgIter &it, ParseResult &result,
		decltype(pos)::const_iterator &posit) const
	{
		if (posit == pos.end())
		{
			out << "Unknown argument \"" << it.arg << '"' << std::endl;
			return result.code = result.unknown;
		}
		if (!(*posit)->parse(it))
		{
			out << "Error parsing positional \""
				<< (*posit)->names[0] << '"' << std::endl;
			return result.code = result.error;
		}
		result.args.insert(*posit);
		++posit;
		return 0;
	}

	void Parser::check_required(
		ParseResult &result,
		decltype(pos)::const_iterator &posit) const
	{
		for (; posit != pos.end(); ++posit)
		{
			if ((*posit)->required)
			{
				out << "Missing required positional argument \""
					<< (*posit)->names[0] << '"' << std::endl;
				result.code = result.missing;
				return;
			}
		}
		for (auto flagpair : flags)
		{
			if (
				flagpair.second->required
				&& result.args.find(flagpair.second) == result.args.end())
			{
				out << "Missing required flag \""
					<< Flagname{prefix, most<gt>(flagpair.second->names)}
					<< '"' << std::endl;
				result.code = result.missing;
				return;
			}
		}
	}

	Group::Group(Parser &parent, const char *name):
		parent(parent),
		name(name),
		members{}
	{ parent.groups.push_back(this); }

	void Group::add(ArgCommon &arg)
	{
		parent.add(arg);
		if (!members.insert(&arg).second)
		{
			throw std::logic_error(
				"Arg already added: " + std::string(arg.names[0]));
		}
	}

	void Group::add(FlagCommon &arg)
	{
		parent.add(arg);
		if (!members.insert(&arg).second)
		{
			std::string msg("Flag already added: ");
			msg += Flagname{parent.prefix, most<gt>(arg.names)};
			throw std::logic_error(msg);
		}
	}

}
