#pragma once

#include <exception>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
namespace cmd {
	class ParseException : std::exception
	{
	public:
		ParseException() = default;
		char const* what() const noexcept override;
	};
	class HelpException : std::exception
	{
	public:
		HelpException() = default;
		char const* what() const noexcept override;
	};
	class parser
	{
		enum class ArgType
		{
			None,
			Help,
			String,
			Int,
			Bool
		};
		typedef std::variant<int, std::string, bool> ArgValue;
	public:
		void parse(const std::vector<std::string>& args);
		void addString(const std::string& name, const char& shortName, const std::string& desc, bool require, const std::string& defaultValue = "");
		void addInt(const std::string& name, const char& shortName, const std::string& desc, bool require, const int& defaultValue = 0);
		void addBool(const std::string& name, const char& shortName, const std::string& desc);
		void addPos(const std::string& name, bool require);
		void addHelp(const std::string& name = "help", const char& shortName = 'h');
		std::string getString(const std::string& name) const;
		int getInt(const std::string& name) const;
		bool getBool(const std::string& name) const;
		std::string getPos(const size_t& index) const;
		std::string useage() const;

	private:
		bool hasName(const std::string& name) const;
		bool hasName(const char& shortName) const;
		std::optional<ArgType> getArgType(const std::string& name) const;
		std::optional<ArgType> getArgType(const char& shortName) const;
		bool setValue(const std::string& name, const std::string& value);
		bool setValue(const char& shortName, const std::string& value);

	private:
		std::map<std::string, ArgValue> parse_options_value;
		std::vector<std::string> parse_pos_value;
		std::vector<std::tuple<std::string, char, std::string, bool, ArgType, ArgValue>> args_;
		std::vector<std::tuple<std::string, bool>> pos_args_;
		std::string programName_;
	};
}
