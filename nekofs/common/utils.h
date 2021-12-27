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

	int32_t istream_read(std::shared_ptr<IStream> is, void* buf, const int32_t& size);
	int32_t ostream_write(std::shared_ptr<OStream> os, const void* buf, const int32_t& size);
	std::array<uint32_t, 8> str_to_sha256(const char* str);
	std::string sha256_to_str(const std::array<uint32_t, 8>& sha256);

	inline bool str_EndWith(const std::string& str, const std::string& endstr)
	{
		return !endstr.empty() && str.size() > endstr.size() && str.rfind(endstr) == str.size() - endstr.size();
	}

	inline bool str_EndWith(const std::string& str, const std::string_view& endstr)
	{
		return !endstr.empty() && str.size() > endstr.size() && str.rfind(endstr) == str.size() - endstr.size();
	}
}
