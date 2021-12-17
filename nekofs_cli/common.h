#pragma once
#include <nekofs/nekofs.h>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstring>
#include <string>
#include <iostream>

inline std::string get_utf8_str(const std::string& str)
{
#ifdef _WIN32
	auto size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring u16str(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), &u16str[0], size_needed);

	size_needed = WideCharToMultiByte(CP_UTF8, 0, &u16str[0], (int)u16str.size(), NULL, 0, NULL, NULL);
	std::string u8str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &u16str[0], (int)u16str.size(), &u8str[0], size_needed, NULL, NULL);
	return u8str;
#else
	return str;
#endif
}

inline std::string get_sys_str(const std::string& str)
{
#ifdef _WIN32
	auto size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring u16str(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &u16str[0], size_needed);

	size_needed = WideCharToMultiByte(CP_ACP, 0, &u16str[0], (int)u16str.size(), NULL, 0, NULL, NULL);
	std::string u8str(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, &u16str[0], (int)u16str.size(), &u8str[0], size_needed, NULL, NULL);
	return u8str;
#else
	return str;
#endif
}

extern "C" {
#ifdef _WIN32
	static inline void default_nekofs_log(NEKOFSLogLevel level, const char* u8str)
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
				std::cout << "\033[33m" << "[WARN]" << "\033[0m" << "  " << astr << std::endl;
				break;
			case NEKOFS_LOGERR:
				std::cerr << "\033[31m" << "[ERRO]" << "\033[0m" << "  " << astr << std::endl;
				break;
			default:
				break;
			}
		}
	}
#else
	static inline void default_nekofs_log(NEKOFSLogLevel level, const char* u8str)
	{
		if (u8str != nullptr)
		{
			switch (level)
			{
			case NEKOFS_LOGINFO:
				std::cout << "[INFO]  " << u8str << std::endl;
				break;
			case NEKOFS_LOGWARN:
				std::cout << "\033[33m" << "[WARN]" << "\033[0m" << "  " << u8str << std::endl;
				break;
			case NEKOFS_LOGERR:
				std::cerr << "\033[31m" << "[ERRO]" << "\033[0m" << "  " << u8str << std::endl;
				break;
			default:
				break;
			}
		}
	}
#endif
	static inline void setlog()
	{
		nekofs_SetLogDelegate(default_nekofs_log);
	}
}
