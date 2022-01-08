#pragma once

#include "typedef.h"
#include "env.h"

#include <array>
#include <string>

namespace nekofs {
	std::string getSysErrMsg();
	void loginfo(const char* message);
	void loginfo(const std::string& message);
	void logwarn(const char* message);
	void logwarn(const std::string& message);
	void logerr(const char* message);
	void logerr(const std::string& message);

	std::array<uint32_t, 8> str_to_sha256(const char* str);
	std::string sha256_to_str(const std::array<uint32_t, 8>& sha256);

	inline int32_t istream_read(std::shared_ptr<IStream> is, void* buf, const int32_t& size)
	{
		if (size < 0 || !is)
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
	inline int32_t ostream_read(std::shared_ptr<OStream> os, void* buf, const int32_t& size)
	{
		if (size < 0 || !os)
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
			actulRead = os->read(buffer, size_tmp);
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
	inline int32_t ostream_write(std::shared_ptr<OStream> os, const void* buf, const int32_t& size)
	{
		if (size < 0 || !os)
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

	inline bool str_EndWith(const std::string& str, const std::string& endstr)
	{
		return !endstr.empty() && str.size() > endstr.size() && str.rfind(endstr) == str.size() - endstr.size();
	}

	inline bool str_EndWith(const std::string& str, const std::string_view& endstr)
	{
		return !endstr.empty() && str.size() > endstr.size() && str.rfind(endstr) == str.size() - endstr.size();
	}
}
