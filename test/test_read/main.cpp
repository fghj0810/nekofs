#include "nekofs.h"

#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef _WIN32
const fschar* fs_filepath = L"D:/test/testfile";
const char* c_filepath = "D:/test/testfile";
#else
const fschar* fs_filepath = "/home/jie/work/testfile";
const char* c_filepath = "/home/jie/work/testfile";
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

int32_t read_nekofs(FILE* f, void* buf, const int32_t& size)
{
	int32_t size_tmp = size;
	uint8_t* buffer = static_cast<uint8_t*>(buf);
	int32_t actulRead = 0;
	do
	{
		actulRead = fread(buffer, 1, size_tmp, f);
		if (actulRead > 0)
		{
			size_tmp -= actulRead;
			buffer += actulRead;
		}
	} while (actulRead > 0 && size_tmp > 0);
	return size - size_tmp;
}

int main()
{
	nekofs_SetLogDelegate(log111);
	auto handle = nekofs_native_OpenIStream(fs_filepath);
	auto cfile = fopen(c_filepath, "rb");
	const size_t testSize = 991;
	auto buf1 = new char[testSize];
	auto buf2 = new char[testSize];
	std::memset(buf1, 0, testSize);
	std::memset(buf2, 0, testSize);
	int32_t ret1, ret2;
	int64_t count = 0;
	do
	{
		count++;
		ret1 = nekofs_istream_Read(handle, buf1, testSize);
		ret2 = read_nekofs(cfile, buf2, testSize);
		if (ret1 != ret2 || std::memcmp(buf1, buf2, testSize) != 0)
		{
			std::cout << "diff " << count << std::endl;
		}
	} while (ret1 > 0 && ret2 > 0);
	if (ret1 != ret2)
	{
		std::cout << "diff" << std::endl;
	}
}
