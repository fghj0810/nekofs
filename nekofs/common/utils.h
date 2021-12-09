#pragma once

#include "typedef.h"
#include "env.h"

namespace nekofs {
	void logprint(const LogType& level, const fschar* message);
	void logprint(const LogType& level, const fsstring& message);
	SeekOrigin Int2SeekOrigin(const NekoFSOrigin& value);

	//class FSID final : private noncopyable, private nonmovable
	//{
	//public:
	//	FSID();
	//	~FSID();
	//	int32_t getId();

	//private:
	//	int32_t id_ = 0;
	//};

	int32_t istream_read(std::shared_ptr<IStream>& is, void* buf, const int32_t& size);
	int32_t ostream_write(std::shared_ptr<OStream>& os, const void* buf, const int32_t& size);
}
