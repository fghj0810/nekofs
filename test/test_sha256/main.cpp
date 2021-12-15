#include "nekofs/nekofs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#ifdef _WIN32
const char* test_file = u8"D:/test/testfile";
#else
const char* test_file = "/home/jie/work/testfile";
#endif

extern "C" {
	void log111(int32_t level, const char* str)
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
}

int main()
{
	nekofs_SetLogDelegate(log111);
	uint32_t result[8];
	auto handle_read = nekofs_native_OpenIStream(test_file);
	nekofs_sha256_sumistream32(handle_read, result);
	for (size_t i = 0; i < 8; i++)
	{
		std::cout << std::hex << std::setw(8) << std::setfill('0') << result[i];
	}
	std::cout << std::endl;
	return 0;
}
