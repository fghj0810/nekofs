#pragma once

#include "typedef.h"
#include "env.h"

namespace nekofs {
	std::string getSysErrMsg();
	void logprint(const LogType& level, const char* message);
	void logprint(const LogType& level, const std::string& message);

	int32_t istream_read(std::shared_ptr<IStream>& is, void* buf, const int32_t& size);
	int32_t ostream_write(std::shared_ptr<OStream>& os, const void* buf, const int32_t& size);
	void str_to_sha256(const char* in, uint32_t out[8]);
	std::string sha256_to_str(const uint32_t out[8]);

}
