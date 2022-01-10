#include "pack.h"
#include "common.h"
#include "cmdparse.h"

#include <nekofs/nekofs.h>
#include <filesystem>
#include <iostream>

namespace nekofs_tool {
	int pack(const std::vector<std::string>& args)
	{
		cmd::parser cp;
		cp.addPos("packpath", true);
		cp.addPos("outfile", true);
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
		std::string path = cp.getPos(0);
		std::string out = cp.getPos(1);
		if (path.empty())
		{
			std::cerr << "path.empty()   " << path << std::endl;
			return -1;
		}
		if (out.empty())
		{
			std::cerr << "out.empty()   " << out << std::endl;
			return -1;
		}
		auto dpath = std::filesystem::absolute(path).lexically_normal().generic_string();
		if (!std::filesystem::is_directory(dpath))
		{
			std::cerr << "!std::filesystem::is_directory(" << dpath << ")" << std::endl;
			return -1;
		}
		dpath = get_utf8_str(dpath);
		auto fpath = std::filesystem::absolute(out).lexically_normal().generic_string();
		if (std::filesystem::exists(fpath))
		{
			std::cerr << "std::filesystem::exists(" << fpath << ")" << std::endl;
			return -1;
		}
		fpath = get_utf8_str(fpath);
		if (NEKOFS_FALSE == nekofs_tools_pack(dpath.c_str(), fpath.c_str()))
		{
			std::cerr << "nekofs_tools_pack error" << std::endl;
			return -1;
		}
		return 0;
	}
}
