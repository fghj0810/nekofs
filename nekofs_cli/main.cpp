#include "common.h"
#include "prepare.h"
#include "pack.h"
#include "unpack.h"
#include "mkdiff.h"

#include <nekofs/nekofs.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

constexpr const char* helpmsg = "\n"
"Available subcommands:\n"
"    prepare\n"
"    pack\n"
"    unpack\n"
"    mkdiff\n"
"    help\n"
"\n";

int main(int argc, char** argv)
{
	setlog();
	if (argc < 2)
	{
		std::cerr << "usage: " << argv[0] << " <subcommand> [options] [args]\n" << helpmsg;
		return -1;
	}
	if (::strcmp("prepare", argv[1]) == 0)
	{
		std::vector<std::string> args(argc - 1);
		args[0] = std::string(argv[0]) + " " + std::string(argv[1]);
		for (int i = 2; i < argc; i++)
		{
			args[i - 1] = argv[i];
		}
		return nekofs_tool::prepare(args);
	}
	if (::strcmp("pack", argv[1]) == 0)
	{
		std::vector<std::string> args(argc - 1);
		args[0] = std::string(argv[0]) + " " + std::string(argv[1]);
		for (int i = 2; i < argc; i++)
		{
			args[i - 1] = argv[i];
		}
		return nekofs_tool::pack(args);
	}
	if (::strcmp("unpack", argv[1]) == 0)
	{
		std::vector<std::string> args(argc - 1);
		args[0] = std::string(argv[0]) + " " + std::string(argv[1]);
		for (int i = 2; i < argc; i++)
		{
			args[i - 1] = argv[i];
		}
		return nekofs_tool::unpack(args);
	}
	if (::strcmp("mkdiff", argv[1]) == 0)
	{
		std::vector<std::string> args(argc - 1);
		args[0] = std::string(argv[0]) + " " + std::string(argv[1]);
		for (int i = 2; i < argc; i++)
		{
			args[i - 1] = argv[i];
		}
		return nekofs_tool::mkldiff(args);
	}
	if (::strcmp("help", argv[1]) == 0)
	{
		std::cout << "usage: " << argv[0] << " <subcommand> [options] [args]\n" << helpmsg;
		return 0;
	}
	std::cerr << "usage: " << argv[0] << " <subcommand> [options] [args]\n" << helpmsg;
	return -1;
}
