#include "nekofs.h"

#include <cstdio>

#include <iostream>

extern "C" {
	void log111(int32_t, fschar* str)
	{
		std::wcout << str << std::endl;
	}
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
	auto handle2 = nekofs_native_OpenOStream(L"D:/test/testfile");
	auto handle3 = nekofs_native_OpenIStream(L"D:/test/testfile");
	auto handle = nekofs_native_OpenIStream(L"D:/test/testfile");
	auto cfile = fopen("D:/test/testfile", "rb");
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
