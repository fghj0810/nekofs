#include "cmdparse.h"

#include <sstream>
#include <charconv>
namespace cmd {
	bool parser::parse(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			return false;
		}
		programName_ = args[0];
		for (size_t i = 1; i < args.size(); i++)
		{
			const auto& item = args[i];
			if (item.find("--") == 0)
			{
				auto pos = item.find("=");
				if (pos != item.npos)
				{
					auto name = item.substr(2, pos - 2);
					auto arg = getArgType(name);
					if (!arg.has_value())
					{
						throw std::exception(kParseError.data());
					}
					if (*arg == ArgType::Help)
					{
						throw std::exception(kHelpError.data());
					}
					if (*arg != ArgType::None)
					{
						if (!setValue(name, item.substr(pos + 1)))
						{
							throw std::exception(kParseError.data());
						}
					}
				}
				else
				{
					auto name = item.substr(2);
					auto arg = getArgType(name);
					if (!arg.has_value())
					{
						throw std::exception(kParseError.data());
					}
					if (*arg == ArgType::Help)
					{
						throw std::exception(kHelpError.data());
					}
					if (*arg != ArgType::None)
					{
						if (i + 1 >= args.size() || !setValue(name, args[i + 1]))
						{
							throw std::exception(kParseError.data());
						}
						i++;
					}
				}
			}
			else if (item.find("-") == 0)
			{
				if (item.size() == 2)
				{
					auto arg = getArgType(item[1]);
					if (!arg.has_value())
					{
						throw std::exception(kParseError.data());
					}
					if (*arg == ArgType::Help)
					{
						throw std::exception(kHelpError.data());
					}
					if (*arg != ArgType::None)
					{
						if (i + 1 >= args.size() || !setValue(item[1], args[i + 1]))
						{
							throw std::exception(kParseError.data());
						}
						i++;
					}
				}
				else
				{
					throw std::exception(kParseError.data());
				}
			}
			else
			{
				parse_pos_value.push_back(item);
			}
		}
	}

	void parser::addString(const std::string& name, const char& shortName, const std::string& desc, bool require, const std::string& defaultValue)
	{
		if (shortName != '\0' && hasName(shortName))
		{
			std::exit(-1);
		}
		if (name.empty() || hasName(name))
		{
			std::exit(-1);
		}
		args_.push_back(std::tuple<std::string, char, std::string, bool, ArgType, ArgValue>(name, shortName, desc, require, ArgType::String, defaultValue));
	}
	void parser::addInt(const std::string& name, const char& shortName, const std::string& desc, bool require, const int& defaultValue)
	{
		if (shortName != '\0' && hasName(shortName))
		{
			std::exit(-1);
		}
		if (name.empty() || hasName(name))
		{
			std::exit(-1);
		}
		args_.push_back(std::tuple<std::string, char, std::string, bool, ArgType, ArgValue>(name, shortName, desc, require, ArgType::Int, defaultValue));
	}
	void parser::addPos(const std::string& name, bool require)
	{
		pos_args_.push_back(std::tuple<std::string, bool>(name, require));
	}
	void parser::addHelp(const std::string& name, const char& shortName)
	{
		if (shortName != '\0' && hasName(shortName))
		{
			std::exit(-1);
		}
		if (name.empty() || hasName(name))
		{
			std::exit(-1);
		}
		args_.push_back(std::tuple<std::string, char, std::string, bool, ArgType, ArgValue>(name, shortName, "print this message", false, ArgType::Help, false));
	}
	std::string parser::getString(const std::string& name) const
	{
		auto it2 = std::find_if(args_.begin(), args_.end(), [name](const auto& rhs)-> bool {
			return name == std::get<0>(rhs);
			}
		);
		if (it2 != args_.end() && std::get<4>(*it2) == ArgType::String)
		{
			auto it1 = parse_options_value.find(name);
			if (it1 != parse_options_value.end())
			{
				return std::get<std::string>(it1->second);
			}
			return std::get<std::string>(std::get<5>(*it2));
		}
		return std::string();
	}
	int parser::getInt(const std::string& name) const
	{
		auto it2 = std::find_if(args_.begin(), args_.end(), [name](const auto& rhs)-> bool {
			return name == std::get<0>(rhs);
			}
		);
		if (it2 != args_.end() && std::get<4>(*it2) == ArgType::Int)
		{
			auto it1 = parse_options_value.find(name);
			if (it1 != parse_options_value.end())
			{
				return std::get<int>(it1->second);
			}
			return std::get<int>(std::get<5>(*it2));
		}
		return 0;
	}
	std::string parser::getPos(const size_t& index) const
	{
		if (index < parse_pos_value.size())
		{
			return parse_pos_value[index];
		}
	}
	std::string parser::useage() const
	{
		std::stringstream ss;
		ss << "usage: " << programName_;
		bool has_optional = false;
		for (const auto& item : args_)
		{
			if (std::get<3>(item))
			{
				ss << " --" << std::get<0>(item);
				switch (std::get<4>(item))
				{
				case ArgType::String:
					ss << "=string";
					break;
				case ArgType::Int:
					ss << "=int";
					break;
				default:
					break;
				}
			}
			else if (std::get<4>(item) != ArgType::Help)
			{
				has_optional = true;
			}
		}
		if (has_optional)
		{
			ss << " [options]";
		}
		for (const auto& item : pos_args_)
		{
			ss << " ";
			if (!std::get<1>(item))
			{
				ss << "[";
			}
			ss << std::get<0>(item);
			if (!std::get<1>(item))
			{
				ss << "]";
			}
		}
		ss << std::endl;
		ss << "options:" << std::endl;
		size_t max_name_length = 0;
		for (const auto& item : args_)
		{
			max_name_length = std::max(max_name_length, std::get<0>(item).size());
		}
		for (const auto& item : args_)
		{
			ss << "  ";
			if (std::get<1>(item) != '\0')
			{
				ss << "-" << std::get<1>(item) << ", ";
			}
			else
			{
				ss << "    ";
			}
			ss << "--" << std::get<0>(item);
			for (size_t i = std::get<0>(item).size(); i < max_name_length + 4; i++)
			{
				ss << ' ';
			}
			ss << std::get<2>(item);
			if (std::get<4>(item) != ArgType::None && std::get<4>(item) != ArgType::Help)
			{
				ss << "(";
				switch (std::get<4>(item))
				{
				case ArgType::String:
					ss << "string";
					if (!std::get<3>(item))
					{
						ss << " =[" << std::get<std::string>(std::get<5>(item)) << "]";
					}
					break;
				case ArgType::Int:
					ss << "int";
					if (!std::get<3>(item))
					{
						ss << " =[" << std::get<int>(std::get<5>(item)) << "]";
					}
					break;
				default:
					break;
				}
				ss << ")";
			}
			ss << std::endl;
		}
		return ss.str();
	}
	bool parser::hasName(const std::string& name) const
	{
		if (!name.empty())
		{
			for (const auto& item : args_)
			{
				if (std::get<0>(item) == name)
				{
					return true;
				}
			}
		}
		return false;
	}
	bool parser::hasName(const char& shortName) const
	{
		if (shortName != '\0')
		{
			for (const auto& item : args_)
			{
				if (std::get<1>(item) == shortName)
				{
					return true;
				}
			}
		}
		return false;
	}
	std::optional<parser::ArgType> parser::getArgType(const std::string& name) const
	{
		if (!name.empty())
		{
			for (const auto& item : args_)
			{
				if (std::get<0>(item) == name)
				{
					return std::get<4>(item);
				}
			}
		}
		return std::nullopt;
	}
	std::optional<parser::ArgType> parser::getArgType(const char& shortName) const
	{
		if (shortName != '\0')
		{
			for (const auto& item : args_)
			{
				if (std::get<1>(item) == shortName)
				{
					return std::get<4>(item);
				}
			}
		}
		return std::nullopt;
	}
	bool parser::setValue(const std::string& name, const std::string& value)
	{
		auto type = getArgType(name);
		if (type.has_value())
		{
			auto it2 = std::find_if(args_.begin(), args_.end(), [name](const auto& rhs)-> bool {
				return name == std::get<0>(rhs);
				}
			);
			int ivalue;
			switch (*type)
			{
			case ArgType::String:
				parse_options_value[name] = value;
				break;
			case ArgType::Int:
				if (std::from_chars(&value[0], &value[value.size() - 1], ivalue, 10).ptr != &value[value.size() - 1])
				{
					return false;
				}
				parse_options_value[name] = ivalue;
				break;
			default:
				break;
			}
			return true;
		}
		return false;
	}
	bool parser::setValue(const char& shortName, const std::string& value)
	{
		auto it2 = std::find_if(args_.begin(), args_.end(), [shortName](const auto& rhs)-> bool {
			return shortName == std::get<1>(rhs);
			}
		);
		if (it2 != args_.end())
		{
			return setValue(std::get<0>(*it2), value);
		}
		return false;
	}
}
