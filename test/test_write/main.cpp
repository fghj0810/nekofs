#include "nekofs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef _WIN32
const fschar* old_file = L"D:/test/testfile";
const fschar* new_filepath = L"D:/test2/testfile2/2/2";
#else
const fschar* old_file = "/home/jie/work/testfile";
const fschar* new_filepath = "/home/jie/work/testfile2/2/2";
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

void copy_test(NekoFSHandle is, NekoFSHandle os)
{
	nekofs_istream_Seek(is, 0, NEKOFS_BEGIN);
	nekofs_ostream_Seek(os, 0, NEKOFS_BEGIN);
	const size_t testSize = 991;
	auto buf = new char[testSize];
	int32_t actual = 0;
	do
	{
		actual = nekofs_istream_Read(is, buf, testSize);
		if (nekofs_ostream_Write(os, buf, actual) != actual)
		{
			break;
		}
	} while (actual > 0);
	delete[] buf;
}

void copy_test_random(NekoFSHandle is, NekoFSHandle os)
{
	nekofs_istream_Seek(is, 0, NEKOFS_BEGIN);
	nekofs_ostream_Seek(os, 0, NEKOFS_BEGIN);
	const size_t testSize = 991;
	auto buf = new char[testSize];
	int32_t actual = 0;
	do
	{
		actual = nekofs_istream_Read(is, buf, testSize);
		if (nekofs_ostream_Write(os, buf, actual) != actual)
		{
			break;
		}
		int offset = rand() % 991;
		nekofs_istream_Seek(is, -offset, NEKOFS_CURRENT);
		nekofs_ostream_Seek(os, -offset, NEKOFS_CURRENT);
	} while (actual > 0);
	delete[] buf;
}

bool compare_stream(NekoFSHandle is1, NekoFSHandle is2)
{
	const size_t testSize = 991;
	auto buf1 = new char[testSize];
	auto buf2 = new char[testSize];
	std::memset(buf1, 0, testSize);
	std::memset(buf2, 0, testSize);
	int32_t ret1, ret2;
	int64_t count = 0;
	do
	{
		ret1 = ret2 = 0;
		count++;
		ret1 = nekofs_istream_Read(is1, buf1, testSize);
		ret2 = nekofs_istream_Read(is2, buf2, testSize);
		if (ret1 != ret2 || std::memcmp(buf1, buf2, testSize) != 0)
		{
			delete[] buf1;
			delete[] buf2;
			return false;
		}
	} while (ret1 > 0 && ret2 > 0);
	delete[] buf1;
	delete[] buf2;
	if (ret1 != ret2)
	{
		return false;
	}
	return true;
}

int main()
{
	nekofs_SetLogDelegate(log111);
	nekofs_native_RemoveFile(new_filepath);
	auto handle_read = nekofs_native_OpenIStream(old_file);
	auto handle_write = nekofs_native_OpenOStream(new_filepath);
	copy_test(handle_read, handle_write);
	nekofs_ostream_Close(handle_write);
	auto handle_read_new = nekofs_native_OpenIStream(new_filepath);
	nekofs_istream_Seek(handle_read, -nekofs_istream_GetLength(handle_read), NEKOFS_CURRENT);
	if (!compare_stream(handle_read, handle_read_new))
	{
		std::cout << "diff1" << std::endl;
		return 1;
	}
	nekofs_istream_Close(handle_read_new);
	nekofs_native_RemoveFile(new_filepath);
	handle_write = nekofs_native_OpenOStream(new_filepath);
	copy_test_random(handle_read, handle_write);
	nekofs_ostream_Close(handle_write);
	handle_read_new = nekofs_native_OpenIStream(new_filepath);
	nekofs_istream_Seek(handle_read, -nekofs_istream_GetLength(handle_read), NEKOFS_END);
	if (!compare_stream(handle_read, handle_read_new))
	{
		std::cout << "diff2" << std::endl;
		return 1;
	}
	return 0;
}
