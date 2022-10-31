//impl for Parser class
#ifndef ARGPARSE_IMPL_HPP
#define ARGPARSE_IMPL_HPP

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace argparse
{
	void Parser::add_flag(const char *rawname)
	{
		if (!flagmap.insert({rawname, flags.size()}).second)
		{
			throw std::logic_error(
				sstr("already added flag name \"") << prefix << rawname << '"');
		}
	}

	void add_ordered(std::vector<const char*> &lst, const char *val)
	{
		auto it = lst.begin();
		auto end = lst.end();
		while (it != end)
		{
			if (strcmp(val, *it) < 0)
			{
				lst.insert(it, val);
				return;
			}
			++it;
		}
		lst.push_back(val);
	}

	std::map<std::size_t, std::vector<const char*>> Parser::names() const
	{
		std::map<std::size_t, std::vector<const char*>> ret;
		for (auto item : flagmap)
		{ add_ordered(ret[item.second], item.first); }
		return ret;
	}


	template<class T, int nargs>
	const decltype(Arg<T, nargs>::value)& Parser::add(
		const std::vector<const char*> &names,
		const char *help,
		typename Arg<T, nargs>::def dval)
	{
		std::vector<BaseArg*> *dst;
		const char *name = names[0];
		if (names[0][0] == prefix)
		{
			for (const char *nm : names)
			{
				if (nm[0] != prefix)
				{
					throw std::logic_error(
						sstr("flag ") << names[0] << " altname \"" << nm
						<< "\" does not begin with '" << prefix << "'.");
				}
				add_flag(rawname(nm, prefix));
				if (strcmp(nm, name) > 0) { name = nm; }
			}
			dst = &flags;
		}
		else
		{
			if (names.size() > 2)
			{
				sstr msg("positional should have only 1 name, but got ");
				msg << names.size() << ": ";
				const char *sep = "";
				for (const char *nm : names)
				{ msg << sep << nm; sep = ", "; }
				throw std::logic_error(msg);
			}
			if (nargs == 0)
			{ throw std::logic_error("positional argument should have at least 1 arg"); }
			dst = &positionals;
		}
		auto *ptr = new Arg<T, nargs>(name, help, dval);
		dst->push_back(ptr);
		return ptr->value;
	}

	template<class T, int nargs>
	const decltype(Arg<T, nargs>::value)& Parser::add(
		const char *name,
		const char *help,
		typename Arg<T, nargs>::def dval)
	{ return add<T, nargs>(std::vector<const char*>{name}, help, dval); }

	Parser::~Parser()
	{
		for (BaseArg *flag : flags)
		{ delete flag; }
		for (BaseArg *pos : positionals)
		{ delete pos; }
	}

	BaseArg* Parser::find_flag(const char *arg) const
	{
		const char *name = rawname(arg, prefix);
		auto it = flagmap.find(name);
		if (it != flagmap.end())
		{ return flags[it->second]; }
		BaseArg *ret = nullptr;
		const char *pick;
		for (const auto &it : flagmap)
		{
			if (startswith(it.first, name))
			{
				if (ret)
				{
					std::cerr << "Ambiguous flag \"" << arg << '"' << std::endl;
					std::cerr << prefix << pick << " vs " << prefix << it.first << std::endl;
					return nullptr;
				}
				ret = flags[it.second];
				pick = it.first;
			}
		}
		if (!ret)
		{ std::cerr << "unknown flag \"" << arg << '"' << std::endl; }
		return ret;
	}

	bool tryparse(BaseArg *arg, ArgIter &args)
	{
		try
		{ arg->parse(args); }
		catch (std::exception &e)
		{
			std::cerr << "error parsing " << arg->name
				<< ": " << e.what() << std::endl;
			return true;
		}
		return false;
	}

	bool Parser::parse(int argc, char *argv[])
	{
		//first search for help to avoid overwriting default values.
		//for help message
		const char *arg;
		ArgIter args(argc, argv, prefix);
		while (arg = args.nextflag())
		{
			if (
				arg[0] == prefix
				&& flagmap.find(arg) == flagmap.end()
				&& startswith("help", rawname(arg, prefix)))
			{ help(); return true; }
		}
		args.reset();
		auto poss = positionals.begin();
		while (arg = args.peek())
		{
			if (arg[0] == prefix && !args.nskips)
			{
				args.next();
				BaseArg *flag;
				if (flag = find_flag(arg))
				{
					if (tryparse(flag, args)) { return true; }
				}
				else
				{ return true; }
			}
			else
			{
				if (poss == positionals.end())
				{
					std::cerr << "unrecognized argument \"" << arg << '"';
					return true;
				}
				if (tryparse(*poss, args)) { return true; }
				++poss;
			}
		}
		while (poss != positionals.end())
		{
			if (!(*poss)->ok())
			{
				std::cerr << "missing arguments for " << (*poss)->name << std::endl;
				return true;
			}
			++poss;
		}
		for (BaseArg *flag : flags)
		{
			if (!flag->ok())
			{
				std::cerr << "missing arguments for " << flag->name << std::endl;
				return true;
			}
		}
		return false;
	}

	void Parser::help() const
	{
		std::cout << "usage: " << program;
		auto mp = names();
		for (std::size_t i=0; i<flags.size(); ++i)
		{
			std::cout << " [-" << mp[i][0];
			BaseArg *ptr = flags[i];
			if (ptr->count() < 0)
			{ std::cout << " ..."; }
			else if (ptr->count() > 0)
			{
				std::cout << " <" << mp[i].back() << '>';
				if (ptr->count() > 1)
				{ std::cout << 'x' << ptr->count(); }
			}
			std::cout << ']';
		}
		for (const auto *pos: positionals)
		{
			std::cout << " <" << pos->name;
			if (pos->count() < 0)
			{ std::cout << " ..."; }
			std::cout << '>';
			if (pos->count() > 1)
			{ std::cout << 'x' << pos->count(); }
		}
		std::cout << std::endl << std::endl << "Flags:" << std::endl;
		for (std::size_t i=0; i<flags.size(); ++i)
		{
			const auto &flagnames = mp[i];
			auto it = flagnames.begin();
			auto end = flagnames.end();
			std::cout << "  [" << prefix << *(it++);
			while (it != end)
			{
				std::cout << " | " << prefix << *(it++);
			}
			int count = flags[i]->count();
			if (count == 0)
			{ std::cout << "] (default: " << flags[i]->str() << ')'; }
			else
			{
				std::cout << " <" << flagnames.back();
				if (count < 0) { std::cout << " ..."; }
				std::cout << '>';
				if (count > 1) { std::cout << 'x' << count; }
				auto defaults = flags[i]->str();
				std::cout << ']';
				if (defaults.size())
				{ std::cout << " (default: " << defaults << ')'; }
			}
			std::cout << std::endl;
			if (flags[i]->help[0] != '\0')
			{ std::cout << "    " << flags[i]->help << std::endl;}
		}
		std::cout << std::endl << "Positionals:" << std::endl;
		for (BaseArg *pos : positionals)
		{
			std::cout << "  <" << pos->name;
			int count = pos->count();
			if (count < 0) { std::cout << " ..."; }
			std::cout << '>';
			if (count > 1) { std::cout << 'x' << count; }
			std::string val = pos->str();
			if (val.size())
			{ std::cout << " (default: " << val << ')'; }
			std::cout << std::endl;
			if (pos->help[0] != '\0')
			{ std::cout << "    " << pos->help << std::endl; }
		}
	}
}
#endif
