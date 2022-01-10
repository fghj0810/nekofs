#include "prepare.h"
#include "common.h"
#include "cmdparse.h"

#include <nekofs/nekofs.h>

#include <filesystem>
#include <iostream>

namespace nekofs_tool {
	int prepare(const std::vector<std::string>& args)
	{
		cmd::parser cp;
		cp.addString("verfile", 'v', "version file path", true, "");
		cp.addInt("offset", '\0', "version offet", false, 0);
		cp.addPos("path", true);
		cp.addHelp();
		try
		{
			cp.parse(args);
		}
		catch (const std::exception& e)
		{
			if (e.what() == cmd::kParseError)
			{
				std::cerr << cp.useage() << std::endl;
				std::exit(-1);
			}
			else if (e.what() == cmd::kHelpError)
			{
				std::cout << cp.useage() << std::endl;
				std::exit(0);
			}
		}
		std::string verfile = cp.getString("verfile");
		std::string path = cp.getPos(0);
		int32_t offset = cp.getInt("offset");
		if (path.empty())
		{
			std::cerr << "path.empty()   " << path << std::endl;
			return -1;
		}
		if (verfile.empty())
		{
			std::cerr << "FLAGS_verfile.empty()   " << verfile << std::endl;
			return -1;
		}
		if (offset < 0)
		{
			std::cerr << "offset < 0   " << offset << std::endl;
			return -1;
		}
		auto genpath = std::filesystem::absolute(path).lexically_normal().generic_string();
		if (!std::filesystem::is_directory(genpath))
		{
			std::cerr << "!std::filesystem::is_directory(" << genpath << ")" << std::endl;
			return -1;
		}
		auto versionpath = std::filesystem::absolute(verfile).lexically_normal().generic_string();
		if (!std::filesystem::is_regular_file(versionpath))
		{
			std::cerr << "!std::filesystem::is_regular_file(" << versionpath << ")" << std::endl;
			return -1;
		}
		genpath = get_utf8_str(genpath);
		versionpath = get_utf8_str(versionpath);
		if (NEKOFS_FALSE == nekofs_tools_prepare(genpath.c_str(), versionpath.c_str(), offset))
		{
			std::cerr << "nekofs_tools_prepare error" << genpath << std::endl;
			return -1;
		}
		return 0;
	}
}
