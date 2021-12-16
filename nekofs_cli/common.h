#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstring>
#include <string>

inline std::string get_utf8_str(const char* str)
{
	if (str == nullptr)
	{
		return std::string();
	}
#ifdef _WIN32
	int strLen = (int)::strlen(str);
	auto size_needed = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
	std::wstring u16str(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, str, strLen, &u16str[0], size_needed);

	size_needed = WideCharToMultiByte(CP_UTF8, 0, &u16str[0], (int)u16str.size(), NULL, 0, NULL, NULL);
	std::string u8str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &u16str[0], (int)u16str.size(), &u8str[0], size_needed, NULL, NULL);
	return u8str;
#else
	return str;
#endif
}
