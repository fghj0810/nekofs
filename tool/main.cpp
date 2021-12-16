#include "common.h"
#include "prepare.h"

#include <nekofs/nekofs.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

extern "C" {
#ifdef _WIN32
	static void default_nekofs_log(int32_t level, const char* u8str)
	{
		if (u8str != nullptr)
		{
			int strLen = (int)::strlen(u8str);
			auto size_needed = MultiByteToWideChar(CP_UTF8, 0, u8str, strLen, NULL, 0);
			std::wstring u16str(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, u8str, strLen, &u16str[0], size_needed);

			size_needed = WideCharToMultiByte(CP_ACP, 0, &u16str[0], (int)u16str.size(), NULL, 0, NULL, NULL);
			std::string astr(size_needed, 0);
			WideCharToMultiByte(CP_ACP, 0, &u16str[0], (int)u16str.size(), &astr[0], size_needed, NULL, NULL);

			switch (level)
			{
			case NEKOFS_LOGINFO:
				std::cout << "[INFO]  " << astr << std::endl;
				break;
			case NEKOFS_LOGWARN:
				std::cout << "[WARN]  " << astr << std::endl;
				break;
			case NEKOFS_LOGERR:
				std::cerr << "[ERRO]  " << astr << std::endl;
				break;
			default:
				break;
			}
		}
	}
#else
	static void default_nekofs_log(int32_t level, const char* u8str)
	{
		if (u8str != nullptr)
		{
			switch (level)
			{
			case NEKOFS_LOGINFO:
				std::cout << "[INFO]  " << u8str << std::endl;
				break;
			case NEKOFS_LOGWARN:
				std::cout << "[WARN]  " << u8str << std::endl;
				break;
			case NEKOFS_LOGERR:
				std::cerr << "[ERRO]  " << u8str << std::endl;
				break;
			default:
				break;
			}
		}
	}
#endif
	static void setlog()
	{
		nekofs_SetLogDelegate(default_nekofs_log);
	}
}

constexpr const char* helpmsg = "\n"
"Available subcommands:\n"
"    prepare\n"
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
	if (::strcmp("help", argv[1]) == 0)
	{
		std::cout << "usage: " << argv[0] << " <subcommand> [options] [args]\n" << helpmsg;
		return 0;
	}
	std::cerr << "usage: " << argv[0] << " <subcommand> [options] [args]\n" << helpmsg;
	return -1;
}
