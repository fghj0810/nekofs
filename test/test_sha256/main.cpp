#include "nekofs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#ifdef _WIN32
const fschar* test_file = L"D:/test/testfile";
#else
const fschar* test_file = "/home/jie/work/testfile";
#endif

extern "C" {
#ifdef _WIN32
	void log111(int32_t level, const fschar* str)
	{
		switch (level)
		{
		case NEKOFS_LOGINFO:
			std::wcout << "[INFO]  " << str << std::endl;
			break;
		case NEKOFS_LOGWARN:
			std::wcout << "[WARN]  " << str << std::endl;
			break;
		case NEKOFS_LOGERR:
			std::wcout << "[ERRO]  " << str << std::endl;
			break;
		default:
			break;
		}
	}
#else
	void log111(int32_t level, const fschar* str)
	{
		switch (level)
		{
		case NEKOFS_LOGINFO:
			std::cout << "[INFO]  " << str << std::endl;
			break;
		case NEKOFS_LOGWARN:
			std::cout << "[WARN]  " << str << std::endl;
			break;
		case NEKOFS_LOGERR:
			std::cout << "[ERRO]  " << str << std::endl;
			break;
		default:
			break;
		}
	}
#endif
}

int main()
{
	nekofs_SetLogDelegate(log111);
	uint8_t result[32];
	auto handle_read = nekofs_native_OpenIStream(test_file);
	nekofs_sha256_sumistream(handle_read, result);
	for (size_t i = 0; i < 32; i++)
	{
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)result[i];
	}
	std::cout << std::endl;
	return 0;
}
