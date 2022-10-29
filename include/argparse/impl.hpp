//impl for Parser class
#ifndef ARGPARSE_IMPL_HPP
#define ARGPARSE_IMPL_HPP


namespace argparse
{
		template<class T, int nargs>
		const decltype(Arg<T, nargs>::value)& Parser::flag(
			std::vector<const char*> names,
			const char *help,
			typename Arg<T, nargs>::def dval)
		{
			for (const char *name : names)
			{
				if (!flagmap.insert({rawname(name, prefix), flags.size()}).second)
				{ throw std::logic_error(std::string("already added ")+=name); }
			}
			auto *ptr = new Arg<T, nargs>(names[0], help, dval);
			flags.push_back(ptr);
			return ptr->value;
		}

		template<class T, int nargs>
		const decltype(Arg<T, nargs>::value)& Parser::flag(
			const char *name,
			const char *help,
			typename Arg<T, nargs>::def dval)
		{
			if (!flagmap.insert({rawname(name, prefix), flags.size()}).second)
			{ throw std::logic_error(std::string("already added ")+=name); }
			flagmap[rawname(name, prefix)] = flags.size();
			auto *ptr = new Arg<T, nargs>(name, help, dval);
			flags.push_back(ptr);
			return ptr->value;
		}

		//positional argument
		template<class T, int nargs>
		const decltype(Arg<T, nargs>::value)& Parser::pos(
			const char *name,
			const char *help,
			typename Arg<T, nargs>::def dval)
		{
			if (nargs == 0)
			{ throw std::logic_error("positional argument cannot have 0 values"); }
			auto *ptr = new Arg<T, nargs>(name, help, dval);
			positionals.push_back(ptr);
			return ptr->value;
		}

		Parser::~Parser()
		{
			for (BaseArg *flag : flags)
			{ delete flag; }
			flags.clear();
			for (BaseArg *pos : positionals)
			{ delete pos; }
			positionals.clear();
		}

		BaseArg* Parser::find_flag(const char *arg) const
		{
			const char *name = rawname(arg, prefix);
			auto it = flagmap.find(name);
			if (it != flagmap.end())
			{ return flags[it->second]; }
			BaseArg *ret = nullptr;
			for (const auto &it : flagmap)
			{
				if (startswith(it.first, name))
				{
					if (ret)
					{
						std::cerr << "Ambiguous flag \"" << arg << '"' << std::endl;
						return nullptr;
					}
					ret = flags[it.second];
				}
			}
			if (!ret)
			{
				if (startswith("help", name) && name[0] != '\0')
				{ std::cout << "would do help" << std::endl; }
				else
				{ std::cerr << "unknown flag \"" << arg << '"' << std::endl; }
			}
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
			ArgIter args(argc, argv, prefix, 0);
			auto poss = positionals.begin();
			const char *arg;
			while (arg = args.peek())
			{
				if (arg[0] == prefix)
				{
					args.next();
					BaseArg *flag;
					if (flag = find_flag(arg))
					{ if (tryparse(flag, args)) { return true; } }
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
			{ if (tryparse(*poss, args)) { return true; } }
			for (BaseArg *flag : flags)
			{
				if (flag->count() > 0)
				{ if (tryparse(flag, args)) { return true; } }
			}
			return false;
		}




}
#endif
//	template<class T, int nargs>
//	struct Arg;
//
//	class Parser
//	{
//		public:
//			Parser(char flag_prefix_='-', std::string program_="program"):
//				prefix(flag_prefix_), program(program_),
//				positionals{}, flags{}, flagmap{}
//			{}
//			//parse should only be called once in lifetime of the parser.
//			//ignore program name when parsing
//			bool parse(int argc, char *argv[]);
//			bool parse_main(int argc, char *argv[])
//			{ program = argv[0]; return parse(argc-1, argv+1); }
//			//name: name of argument, start with flag_prefix if a flag.
//			//help: a help string
//			//shortname: short name for the arg, if any.
//			template<class T, int nargs=0>
//			const Arg<T, nargs>& add(
//				const std::string &name="",
//				const std::string &help="",
//				const std::string &shortname="",
//				const std::vector<T> &defaults={});
//			~Parser();
//			char prefix;
//			std::string program;
//			void help() const;
//		private:
//			std::vector<BaseArg*> positionals;
//			std::vector<BaseArg*> flags;
//			std::map<std::string, std::size_t> flagmap;
//
//			void process_arg(int &argc, char **&argv, BaseArg *arg, int &nskips)
//			{
//				int consumed = arg->parse(argc, argv, prefix, nskips);
//				argc -= consumed;
//				argv += consumed;
//			}
//			void process_flag(
//				int &argc, char **&argv, std::vector<BaseArg*>::iterator &it, int &nskips);
//			void process_positional(
//				int &argc, char **&argv, std::vector<BaseArg*>::iterator &it, int &nskips);
//	};
//
//	//------------------------------
//	//basic values
//	//------------------------------
//	template<class T>
//	struct Value;
//
//	template<>
//	struct Value<int>
//	{
//		const char *name;
//		long long value;
//		Value(): name(""), value(0) {}
//		operator int() const { return as<int>(); }
//		operator long() const { return as<long>(); }
//		operator long long() const { return value; }
//		operator unsigned int() const { return as<unsigned int>(); }
//		operator unsigned long() const { return as<unsigned long>(); }
//		operator unsigned long long() const { return as<unsigned long long>(); }
//		template<class T=int>
//		T as() const
//		{
//			if (std::is_unsigned<T>::value and value < 0)
//			{
//				std::stringstream s;
//				s << name << " expected a positive number, but got " << value << ".";
//				throw std::out_of_range(s.str());
//			}
//			return static_cast<T>(value);
//		}
//		void parse(const char *arg) {
//			try
//			{ value = std::stoll(arg); }
//			catch (std::exception&)
//			{
//				std::stringstream s;
//				s << name << " expected a number, but got \"" << arg << '"';
//				throw std::invalid_argument(s.str());
//			}
//		}
//	};
//	template<>
//	struct Value<float>
//	{
//		const char *name;
//		double value;
//		Value(): name(""), value(0.0) {}
//		operator float() const { return as<float>(); }
//		operator double() const { return value; }
//		template<class T=float>
//		T as() const
//		{ return static_cast<T>(value); }
//		void parse(const char *arg)
//		{
//			try
//			{ value = std::stod(arg); }
//			catch (std::exception&)
//			{
//				std::stringstream s;
//				s << name << " expected a float, but got \"" << arg << '"';
//				throw std::invalid_argument(s.str());
//			}
//		}
//	};
//	template<>
//	struct Value<char*>
//	{
//		const char *name;
//		const char *value;
//		Value(): name(""), value("") {}
//		operator const char*() const { return value; }
//		void parse(const char *arg) { value = arg; }
//	};
//
//	//take string after beginning prefix
//	int skipcount(const char *s,  char prefix)
//	{
//		if (s[0] == '\0')
//		{ return 0; }
//		if (s[1] == '\0' && s[0] == prefix)
//		{ return -1; }
//		else if (s[0] == '_')
//		{
//			try
//			{ return std::stoi(s+1); }
//			catch (std::exception&)
//			{ return 0; }
//		}
//		return 0;
//	}
//
//	//------------------------------
//	//argument counts
//	//------------------------------
//	//fixed number of arguments
//	template<class T, int nargs>
//	struct Arg: public BaseArg
//	{
//		Value<T> values[nargs];
//		Arg(std::string name_, std::string help_="", const std::vector<T> &defaults={}):
//			BaseArg(name_, help_),
//			values{}
//		{
//			for (std::size_t i=0; i<nargs; ++i)
//			{
//				values[i].name = name.c_str();
//				if (i < defaults.size())
//				{ values[i].value = static_cast<decltype(Value<T>::value)>(defaults[i]); }
//			}
//		}
//		const Value<T>* begin() const
//		{ return values; }
//		const Value<T>* end() const
//		{ return values+nargs; }
//
//		virtual int parse(int argc, char *argv[], char prefix, int &nskips)
//		{
//			Value<T> *ptr = values;
//			Value<T> *end = values+nargs;
//			int consumed = 0;
//			while (ptr != end && consumed < argc)
//			{
//				char *cur = argv[consumed];
//				++consumed;
//				if (cur[0] == prefix)
//				{
//					if (nskips) { --nskips; }
//					else
//					{
//						if (nskips = skipcount(cur+1, prefix))
//						{ continue; }
//						else
//						{
//							std::stringstream s;
//							s << name << " expected " << nargs << " arguments, but got "
//								<< static_cast<int>(ptr - values) << ".";
//							throw std::runtime_error(s.str());
//						}
//					}
//				}
//				ptr->parse(cur);
//				++ptr;
//			}
//			if (ptr != end)
//			{
//				std::stringstream s;
//				s << name << " expected " << nargs << " arguments, but got "
//					<< static_cast<int>(ptr - values) << ".";
//				throw std::runtime_error(s.str());
//			}
//			return consumed;
//		}
//		virtual int count() const { return nargs; }
//		std::size_t size() const { return static_cast<std::size_t>(nargs); }
//		const Value<T>& operator[](std::size_t idx) const
//		{ return values[idx]; }
//	};
//
//	//flag, no args
//	template<class T>
//	struct Arg<T, 0>: public BaseArg
//	{
//		bool value; 
//		Arg(std::string name_, std::string help_="", const std::vector<T> &defaults={}):
//			BaseArg(name_, help_),
//			value(defaults.size() ? static_cast<bool>(defaults[0]) : false)
//		{}
//		virtual int parse(int argc, char *argv[], char prefix, int &nskips)
//		{
//			value = !value;
//			return 0;
//		}
//		bool operator[](std::size_t idx) const
//		{ return value; }
//		operator bool() const { return value; }
//		virtual int count() const { return 0; }
//		std::size_t size() const { return 0; }
//	};
//
//	//variable args
//	template<class T>
//	struct Arg<T, -1>: public BaseArg
//	{
//		std::vector<Value<T>> values;
//		std::vector<T> defaults;
//		Arg(std::string name_, std::string help_="", const std::vector<T> &defaults_={}):
//			BaseArg(name_, help_),
//			defaults(defaults_)
//		{}
//		const typename std::vector<Value<T>>::const_iterator begin() const
//		{ return values.begin(); }
//		const typename std::vector<Value<T>>::const_iterator end() const
//		{ return values.end(); }
//		virtual int parse(int argc, char *argv[], char prefix, int &nskips)
//		{
//			for (int i=0; i<argc; ++i)
//			{
//				if (argv[i][0] == prefix)
//				{
//					if (nskips) { --nskips; }
//					else
//					{
//						if (nskips = skipcount(argv[i]+1, prefix))
//						{ continue; }
//						else
//						{ return i; }
//					}
//				}
//				values.push_back({});
//				values.back().name = name.c_str();
//				values.back().parse(argv[i]);
//			}
//			if (!values.size())
//			{
//				for (const auto &d : defaults)
//				{
//					values.push_back({});
//					values.back().name = name.c_str();
//					values.back().value = static_cast<decltype(Value<T>::value)>(d);
//				}
//			}
//			return argc;
//		}
//		virtual int count() const { return -1; }
//		std::size_t size() const { return values.size(); }
//		const Value<T>& operator[](std::size_t idx) const
//		{ return values[idx]; }
//	};
//
//	template<class T, int nargs>
//	const Arg<T, nargs>& Parser::add(
//		const std::string &name,
//		const std::string &help,
//		const std::string &shortname,
//		const std::vector<T> &defaults)
//	{
//		BaseArg **bptr;
//		if (name[0] == prefix)
//		{
//			if (!flagmap.insert({name, flags.size()}).second)
//			{
//				std::stringstream s;
//				s << "Argument " << name << " already added!";
//				throw std::logic_error(s.str());
//			}
//			if (shortname.size() && !flagmap.insert({shortname, flags.size()}).second)
//			{
//				flagmap.erase(name);
//				std::stringstream s;
//				s << "Shortname " << shortname << " already exists!";
//				throw std::logic_error(s.str());
//			}
//			flags.push_back(nullptr);
//			bptr = &flags.back();
//		}
//		else
//		{
//			positionals.push_back(nullptr);
//			bptr = &positionals.back();
//		}
//		auto *ptr = new Arg<T, nargs>(
//			name.size() ? name : "arg" + std::to_string(positionals.size()-1),
//			help,
//			defaults);
//		*bptr = ptr;
//		return *ptr;
//	}
//	Parser::~Parser()
//	{
//		for (auto *ptr : flags)
//		{ if (ptr) { delete ptr; } }
//		flags.clear();
//		for (auto *ptr : positionals)
//		{ if (ptr) { delete ptr; } }
//		positionals.clear();
//	}
//
//	std::string rawname(const std::string &s, char prefix)
//	{
//		std::size_t offset = s.find_first_not_of(prefix);
//		if (offset != s.npos)
//		{ return s.substr(offset); }
//		return "";
//	}
//
//	void Parser::process_flag(
//		int &argc, char **&argv, std::vector<BaseArg*>::iterator &it, int &nskips)
//	{
//		if (nskips)
//		{
//			--nskips;
//			process_positional(argc, argv, it, nskips);
//		}
//		else
//		{
//			std::size_t idx;
//			try
//			{ idx = flagmap.at(argv[0]); }
//			catch (std::out_of_range &e)
//			{
//				if (nskips = skipcount(argv[0]+1, prefix))
//				{
//					--argc;
//					++argv;
//					return;
//				}
//				std::string raw = rawname(argv[0], prefix);
//				if (raw == "help" || raw == "h")
//				{ argc = -1; return; }
//				throw std::out_of_range("unrecognized flag " + std::string(argv[0]));
//			}
//			--argc;
//			++argv;
//			process_arg(argc, argv, flags[idx], nskips);
//		}
//
//	}
//	void Parser::process_positional(
//		int &argc, char **&argv, std::vector<BaseArg*>::iterator &it, int &nskips)
//	{
//		if (it == positionals.end())
//		{
//			throw std::runtime_error(
//				"unrecognized positional argument " + std::string(argv[0]));
//		}
//		process_arg(argc, argv, *it, nskips);
//		++it;
//	}
//
//	bool Parser::parse(int argc, char **argv)
//	{
//		auto it = positionals.begin();
//		int nskips = 0;
//		while (argc>0)
//		{
//			if (argv[0][0] == prefix)
//			{ process_flag(argc, argv, it, nskips); }
//			else
//			{ process_positional(argc, argv, it, nskips); }
//		}
//		if (argc<0) { help(); return true; }
//		while (it != positionals.end())
//		{
//			(*it)->parse(0, argv, prefix, nskips);
//			++it;
//		}
//		return false;
//	}
//	struct lensort
//	{
//		bool operator()(
//			const std::string &s1, const std::string &s2) const
//		{ return s1.size() < s2.size(); }
//
//	};
//	void Parser::help() const
//	{
//		std::cout << program;
//		std::map<const BaseArg*, std::vector<std::string>> idx2extras;
//		for (auto it: flagmap)
//		{
//			if (it.first != flags[it.second]->name)
//			{ idx2extras[flags[it.second]].push_back(it.first); }
//		}
//		for (const BaseArg *flag: flags)
//		{
//			std::cout << " [" << (idx2extras[flag].size() ? idx2extras[flag][0] : flag->name);
//			if (flag->count() > 0)
//			{ std::cout << " <" << rawname(flag->name, prefix) << ">x" << flag->count(); }
//			else if (flag->count() < 0)
//			{ std::cout << " <" << rawname(flag->name, prefix) << ">..."; }
//			std::cout << ']';
//		}
//		for (const BaseArg *ptr : positionals)
//		{
//			std::cout << " <" << ptr->name;
//			if (ptr->count() > 1)
//			{ std::cout << ' ' << ptr->count(); }
//			else if (ptr->count() < 0)
//			{ std::cout << " ..."; }
//			std::cout << ">";
//		}
//		std::cout << std::endl;
//		for (const BaseArg *flag: flags)
//		{
//			std::cout << flag->name;
//			if (idx2extras[flag].size())
//			{
//				for (auto &name : idx2extras[flag])
//				{ std::cout << ", " << name; }
//			}
//			if (flag->count() > 0)
//			{ std::cout << " <" << rawname(flag->name, prefix) << ">x" << flag->count(); }
//			else if (flag->count() < 0)
//			{ std::cout << " <" << rawname(flag->name, prefix) << ">..."; }
//			std::cout << std::endl;
//			if (flag->help.size())
//			{ std::cout << '\t' << flag->help << std::endl; }
//		}
//		for (const BaseArg *pos: positionals)
//		{
//			std::cout << "<" << pos->name << '>';
//			if (pos->count() > 1)
//			{ std::cout << " x" << pos->count(); }
//			else if (pos->count() < 0)
//			{ std::cout << "..."; }
//			std::cout << std::endl;
//			if (pos->help.size())
//			{ std::cout << '\t' << pos->help << std::endl; }
//		}
//	}
