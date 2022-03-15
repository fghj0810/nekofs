#include "merge.h"
#include "common.h"
#include "cmdparse.h"

#include <nekofs/nekofs.h>
#include <filesystem>
#include <iostream>

namespace nekofs_tool {
	int merge(const std::vector<std::string>& args)
	{
		cmd::parser cp;
		cp.addString("volumesize", '\0', "volume size (max:3PB)", false, "1MB");
		cp.addBool("noverify", '\0', "verify nekodata");
		cp.addBool("dir", 'd', "output is dir");
		cp.addPos("filename(.nekodata)", true);
		cp.addPos("patchfiles...(.nekodata)", true);
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
		std::vector<std::string> patchlist;
		size_t i = 1;
		for (std::string patchfile = cp.getPos(i); !patchfile.empty(); i++, patchfile = cp.getPos(i))
		{
			patchfile = std::filesystem::absolute(patchfile).lexically_normal().generic_string();
			if (!std::filesystem::is_regular_file(patchfile))
			{
				std::cerr << "!std::filesystem::is_regular_file(" << patchfile << ")" << std::endl;
				return -1;
			}
			patchfile = get_utf8_str(patchfile);
			patchlist.push_back(patchfile);
		}
		if (filename.empty())
		{
			std::cerr << "filename.empty()   " << filename << std::endl;
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
		if (patchlist.empty())
		{
			std::cerr << "no patch files" << std::endl;
			return -1;
		}
		std::vector<const char*> list(patchlist.size(), nullptr);
		for (size_t j = 0; j < list.size(); j++)
		{
			list[j] = patchlist[j].c_str();
		}
		if (cp.getBool("dir"))
		{
			if (NEKOFS_FALSE == nekofs_tools_mergeToDir(filename.c_str(), volumeSize, &list[0], static_cast<int32_t>(list.size()), cp.getBool("noverify") ? NEKOFS_FALSE : NEKOFS_TRUE))
			{
				std::cerr << "nekofs_tools_pack error" << std::endl;
				return -1;
			}
		}
		else
		{
			if (NEKOFS_FALSE == nekofs_tools_mergeToNekodata(filename.c_str(), volumeSize, &list[0], static_cast<int32_t>(list.size()), cp.getBool("noverify") ? NEKOFS_FALSE : NEKOFS_TRUE))
			{
				std::cerr << "nekofs_tools_pack error" << std::endl;
				return -1;
			}
		}
		return 0;
	}
}
