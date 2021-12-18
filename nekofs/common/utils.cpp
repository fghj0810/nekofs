#include "typedef.h"
#include "env.h"
#include "utils.h"

#ifdef _WIN32
#include <Windows.h>
std::string nekofs::getSysErrMsg()
{
	DWORD err = GetLastError();
	LPWSTR msgBuffer = NULL;
	auto msgSize = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL);
	if (msgSize > 0)
	{
		auto size_needed = WideCharToMultiByte(CP_UTF8, 0, msgBuffer, msgSize, NULL, 0, NULL, NULL);
		std::string retvalue(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, msgBuffer, msgSize, &retvalue[0], size_needed, NULL, NULL);
		LocalFree(msgBuffer);
		return retvalue;
	}
	return std::string();
}
#else
#include <cstring>
std::string nekofs::getSysErrMsg()
{
	return std::strerror(errno);
}
#endif // _WIN32


namespace nekofs {
	static inline void logprint(const NEKOFSLogLevel& level, const char* message)
	{
		auto cb = env::getInstance().getLogDelegate();
		if (cb != nullptr)
		{
			cb(level, message);
		}
	}
	void loginfo(const char* message)
	{
		logprint(NEKOFS_LOGINFO, message);
	}
	void loginfo(const std::string& message)
	{
		logerr(message.c_str());
	}
	void logwarn(const char* message)
	{
		logprint(NEKOFS_LOGWARN, message);
	}
	void logwarn(const std::string& message)
	{
		logwarn(message.c_str());
	}
	void logerr(const char* message)
	{
		logprint(NEKOFS_LOGERR, message);
	}
	void logerr(const std::string& message)
	{
		logerr(message.c_str());
	}

	int32_t istream_read(std::shared_ptr<IStream>& is, void* buf, const int32_t& size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		int32_t size_tmp = size;
		uint8_t* buffer = static_cast<uint8_t*>(buf);
		int32_t actulRead = 0;
		do
		{
			actulRead = is->read(buffer, size_tmp);
			if (actulRead > 0)
			{
				size_tmp -= actulRead;
				buffer += actulRead;
			}
		} while (actulRead > 0 && size_tmp > 0);
		if (actulRead < 0 && size == size_tmp)
		{
			return actulRead;
		}
		return size - size_tmp;
	}

	int32_t ostream_write(std::shared_ptr<OStream>& os, const void* buf, const int32_t& size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		int32_t size_tmp = size;
		const uint8_t* buffer = static_cast<const uint8_t*>(buf);
		int32_t actulWrite = 0;
		do
		{
			actulWrite = os->write(buffer, size_tmp);
			if (actulWrite > 0)
			{
				size_tmp -= actulWrite;
				buffer += actulWrite;
			}
		} while (actulWrite > 0 && size_tmp > 0);
		if (actulWrite < 0 && size == size_tmp)
		{
			return actulWrite;
		}
		return size - size_tmp;
	}

	static inline uint8_t hextob(char ch)
	{
		if (ch >= '0' && ch <= '9') return ch - '0';
		if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
		if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
		return 0;
	}
	std::array<uint32_t, 8> str_to_sha256(const char* str)
	{
		std::array<uint32_t, 8> sha256;
		for (size_t i = 0; i < sha256.size(); i++, str += 8)
		{
			sha256[i] = (sha256[i] << 4) | hextob(str[0]);
			sha256[i] = (sha256[i] << 4) | hextob(str[1]);
			sha256[i] = (sha256[i] << 4) | hextob(str[2]);
			sha256[i] = (sha256[i] << 4) | hextob(str[3]);
			sha256[i] = (sha256[i] << 4) | hextob(str[4]);
			sha256[i] = (sha256[i] << 4) | hextob(str[5]);
			sha256[i] = (sha256[i] << 4) | hextob(str[6]);
			sha256[i] = (sha256[i] << 4) | hextob(str[7]);
		}
		return sha256;
	}
	std::string sha256_to_str(const std::array<uint32_t, 8>& sha256)
	{
		const char kh[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
		std::string str(64, 0);
		for (size_t i = 0; i < sha256.size(); i++)
		{
			str[0 + i * 8] = kh[(sha256[i] & 0xF0000000) >> 28];
			str[1 + i * 8] = kh[(sha256[i] & 0x0F000000) >> 24];
			str[2 + i * 8] = kh[(sha256[i] & 0x00F00000) >> 20];
			str[3 + i * 8] = kh[(sha256[i] & 0x000F0000) >> 16];
			str[4 + i * 8] = kh[(sha256[i] & 0x0000F000) >> 12];
			str[5 + i * 8] = kh[(sha256[i] & 0x00000F00) >> 8];
			str[6 + i * 8] = kh[(sha256[i] & 0x000000F0) >> 4];
			str[7 + i * 8] = kh[(sha256[i] & 0x0000000F) >> 0];
		}
		return str;
	}
}

