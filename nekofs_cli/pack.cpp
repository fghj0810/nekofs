#include "pack.h"
#include "common.h"

#include <nekofs/nekofs.h>
#include <gflags/gflags.h>
#include <filesystem>
#include <iostream>

DEFINE_string(packpath, "", "prepare dir path");
DEFINE_string(packout, "", "nekodata file path");

namespace nekofs_tool {
	int pack(const std::vector<std::string>& args)
	{
		int argc = static_cast<int>(args.size());
		char** argv = nullptr;
		std::vector<char*> argv_t(args.size());
		for (size_t i = 0; i < args.size(); i++)
		{
			argv_t[i] = const_cast<char*>(args[i].c_str());
		}
		argv = &argv_t[0];
		gflags::ParseCommandLineFlags(&argc, &argv, false);
		if (FLAGS_packpath.empty())
		{
			std::cerr << "FLAGS_packpath.empty()   " << FLAGS_packpath << std::endl;
			return -1;
		}
		auto dpath = std::filesystem::absolute(FLAGS_packpath).lexically_normal().generic_string();
		if (!std::filesystem::is_directory(dpath))
		{
			std::cerr << "!std::filesystem::is_directory(" << dpath << ")" << std::endl;
			return -1;
		}
		dpath = get_utf8_str(dpath);
		if (FLAGS_packout.empty())
		{
			std::cerr << "FLAGS_packout.empty()   " << FLAGS_packout << std::endl;
			return -1;
		}
		auto fpath = std::filesystem::absolute(FLAGS_packout).lexically_normal().generic_string();
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
