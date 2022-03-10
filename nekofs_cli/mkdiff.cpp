#include "mkdiff.h"
#include "common.h"
#include "cmdparse.h"

#include <nekofs/nekofs.h>
#include <filesystem>
#include <iostream>

namespace nekofs_tool {
	int mkldiff(const std::vector<std::string>& args)
	{
		cmd::parser cp;
		cp.addString("volumesize", '\0', "volume size (max:3PB)", false, "1MB");
		cp.addPos("filename(.nekodata)", true);
		cp.addPos("earlierfile(.nekodata)", true);
		cp.addPos("latestfile(.nekodata)", true);
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
		std::string filename = cp.getPos(0);
		std::string earlierfile = cp.getPos(1);
		std::string latestfile = cp.getPos(2);
		if (filename.empty())
		{
			std::cerr << "filename.empty()   " << filename << std::endl;
			return -1;
		}
		if (earlierfile.empty())
		{
			std::cerr << "earlierfile.empty()   " << earlierfile << std::endl;
			return -1;
		}
		if (latestfile.empty())
		{
			std::cerr << "latestfile.empty()   " << latestfile << std::endl;
			return -1;
		}
		auto vsize = cp.getString("volumesize");
		int64_t volumeSize = getVolumeSizeFromString(vsize);
		if (volumeSize <= 0)
		{
			std::cerr << "volumesize error" << vsize << std::endl;
			return -1;
		}
		filename = std::filesystem::absolute(filename).lexically_normal().generic_string();
		if (std::filesystem::exists(filename))
		{
			std::cerr << "std::filesystem::exists(" << filename << ")" << std::endl;
			return -1;
		}
		filename = get_utf8_str(filename);
		earlierfile = std::filesystem::absolute(earlierfile).lexically_normal().generic_string();
		if (!std::filesystem::is_regular_file(earlierfile))
		{
			std::cerr << "!std::filesystem::is_regular_file(" << earlierfile << ")" << std::endl;
			return -1;
		}
		earlierfile = get_utf8_str(earlierfile);
		latestfile = std::filesystem::absolute(latestfile).lexically_normal().generic_string();
		if (!std::filesystem::is_regular_file(latestfile))
		{
			std::cerr << "!std::filesystem::is_regular_file(" << latestfile << ")" << std::endl;
			return -1;
		}
		latestfile = get_utf8_str(latestfile);
		if (NEKOFS_FALSE == nekofs_tools_mkldiff(earlierfile.c_str(), latestfile.c_str(), filename.c_str(), volumeSize))
		{
			std::cerr << "nekofs_tools_pack error" << std::endl;
			return -1;
		}
		return 0;
	}
}
