#include "pack.h"
#include "common.h"
#include "cmdparse.h"

#include <nekofs/nekofs.h>
#include <filesystem>
#include <iostream>

namespace nekofs_tool {
	int unpack(const std::vector<std::string>& args)
	{
		cmd::parser cp;
		cp.addPos("nekodata", true);
		cp.addPos("outpath", true);
		cp.addHelp();
		try
		{
			cp.parse(args);
		}
		catch (const cmd::ParseException&)
		{
			std::cerr << cp.useage() << std::endl;
			std::exit(-1);
		}
		catch (const cmd::HelpException&)
		{
			std::cout << cp.useage() << std::endl;
			std::exit(0);
		}
		std::string nekodata = cp.getPos(0);
		std::string path = cp.getPos(1);
		if (nekodata.empty())
		{
			std::cerr << "nekodata.empty()   " << nekodata << std::endl;
			return -1;
		}
		if (path.empty())
		{
			std::cerr << "path.empty()   " << path << std::endl;
			return -1;
		}
		nekodata = std::filesystem::absolute(nekodata).lexically_normal().generic_string();
		if (!std::filesystem::is_regular_file(nekodata))
		{
			std::cerr << "!std::filesystem::is_regular_file(" << nekodata << ")" << std::endl;
			return -1;
		}
		nekodata = get_utf8_str(nekodata);
		path = std::filesystem::absolute(path).lexically_normal().generic_string();
		if (std::filesystem::exists(path))
		{
			std::cerr << "std::filesystem::exists(" << path << ")" << std::endl;
			return -1;
		}
		path = get_utf8_str(path);
		if (NEKOFS_FALSE == nekofs_tools_unpack(nekodata.c_str(), path.c_str()))
		{
			std::cerr << "nekofs_tools_unpack error" << std::endl;
			return -1;
		}
		return 0;
	}
}
