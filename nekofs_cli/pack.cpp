#include "pack.h"
#include "common.h"
#include "cmdparse.h"

#include <nekofs/nekofs.h>
#include <filesystem>
#include <iostream>
#include <charconv>

namespace nekofs_tool {
	int pack(const std::vector<std::string>& args)
	{
		cmd::parser cp;
		cp.addString("volumesize", '\0', "volume size (max:1GB)", false, "1MB");
		cp.addPos("outfile", true);
		cp.addPos("packpath", true);
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
		std::string out = cp.getPos(0);
		std::string path = cp.getPos(1);
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
		auto vsize = cp.getString("volumesize");
		int volumeSize = 0;
		{
			if (str_EndWith(vsize, std::string("GB")))
			{
				if (std::from_chars(&vsize[0], &vsize[vsize.size() - 2], volumeSize, 10).ptr != &vsize[vsize.size() - 2])
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				if (volumeSize <= 0 || volumeSize > 1)
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				volumeSize <<= 30;
			}
			else if (str_EndWith(vsize, std::string("MB")))
			{
				if (std::from_chars(&vsize[0], &vsize[vsize.size() - 2], volumeSize, 10).ptr != &vsize[vsize.size() - 2])
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				if (volumeSize <= 0 || volumeSize > (1 << 10))
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				volumeSize <<= 20;
			}
			else if (str_EndWith(vsize, std::string("KB")))
			{
				if (std::from_chars(&vsize[0], &vsize[vsize.size() - 2], volumeSize, 10).ptr != &vsize[vsize.size() - 2])
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				if (volumeSize <= 0 || volumeSize > (1 << 20))
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				volumeSize <<= 10;
			}
			else
			{
				if (std::from_chars(&vsize[0], &vsize[vsize.size()], volumeSize, 10).ptr != &vsize[vsize.size()])
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
				if (volumeSize <= 0 || volumeSize > (1 << 30))
				{
					std::cerr << "volumesize error" << vsize << std::endl;
					return -1;
				}
			}
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
		if (NEKOFS_FALSE == nekofs_tools_pack(dpath.c_str(), fpath.c_str(), volumeSize))
		{
			std::cerr << "nekofs_tools_pack error" << std::endl;
			return -1;
		}
		return 0;
	}
}
