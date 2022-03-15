#include "typedef.h"
#include "env.h"
#include "utils.h"
#include "sha256.h"

#include <mutex>

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
		static std::mutex mtx_log;
		auto cb = env::getInstance().getLogDelegate();
		if (cb != nullptr)
		{
			std::lock_guard lock(mtx_log);
			cb(level, message);
		}
	}
	void loginfo(const char* message)
	{
		logprint(NEKOFS_LOGINFO, message);
	}
	void loginfo(const std::string& message)
	{
		loginfo(message.c_str());
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

	bool verifySHA256(std::shared_ptr<IStream> is, const std::array<uint32_t, 8> sha256)
	{
		if (is)
		{
			nekofs::sha256sum hash;
			auto buffer = env::getInstance().newBufferBlockSize();
			const int32_t buffer_length = static_cast<int32_t>(buffer->size());
			int32_t actualRead = 0;
			do
			{
				actualRead = istream_read(is, buffer->data(), buffer_length);
				if (actualRead > 0)
				{
					hash.update(buffer->data(), actualRead);
				}
			} while (actualRead == buffer_length);
			if (actualRead >= 0)
			{
				hash.final();
				return sha256 == hash.readHash();
			}
		}
		return false;
	}
	bool copyfile(std::shared_ptr<IStream> is, std::shared_ptr<OStream> os)
	{
		if (is == nullptr || os == nullptr)
		{
			return false;
		}
		auto buffer = env::getInstance().newBuffer4M();
		int32_t actualRead = 0;
		int32_t actualWrite = 0;
		int32_t buffer_size = static_cast<int32_t>(buffer->size());
		do
		{
			actualWrite = 0;
			actualRead = istream_read(is, buffer->data(), buffer_size);
			if (actualRead > 0)
			{
				actualWrite = ostream_write(os, buffer->data(), actualRead);
			}
		} while (actualRead > 0 && actualRead == buffer_size && actualRead == actualWrite);
		if (actualRead < 0 || actualRead != actualWrite)
		{
			return false;
		}
		return true;
	}
}

