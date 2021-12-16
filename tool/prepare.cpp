#include "prepare.h"
#include "common.h"

#include <nekofs/nekofs.h>
#include <gflags/gflags.h>
#include <filesystem>
#include <iostream>

DEFINE_string(path, "", "prepare dir path");
DEFINE_string(verfile, "", "version file path");
DEFINE_int32(offset, 0, "version offet");

namespace nekofs_tool {
	int prepare(const std::vector<std::string>& args)
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
		if (FLAGS_path.empty())
		{
			std::cerr << "FLAGS_path.empty()   " << FLAGS_path << std::endl;
			return -1;
		}
		if (FLAGS_verfile.empty())
		{
			std::cerr << "FLAGS_verfile.empty()   " << FLAGS_verfile << std::endl;
			return -1;
		}
		auto genpath = std::filesystem::absolute(FLAGS_path).lexically_normal().generic_string();
		if (!std::filesystem::is_directory(genpath))
		{
			std::cerr << "!std::filesystem::is_directory(" << genpath << ")" << std::endl;
			return -1;
		}
		auto versionpath = std::filesystem::absolute(FLAGS_verfile).lexically_normal().generic_string();
		if (!std::filesystem::is_regular_file(versionpath))
		{
			std::cerr << "!std::filesystem::is_regular_file(" << versionpath << ")" << std::endl;
			return -1;
		}
		genpath = get_utf8_str(genpath.c_str());
		if (NEKOFS_FALSE == nekofs_tools_prepare(genpath.c_str(), versionpath.c_str(), FLAGS_offset))
		{
			std::cerr << "nekofs_tools_prepare error" << genpath << std::endl;
			return -1;
		}
		return 0;
	}
}
