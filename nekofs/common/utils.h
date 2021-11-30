#pragma once

#include "typedef.h"
#include "env.h"

namespace nekofs {
	void logprint(const LogType& level, const fschar* message);
	void logprint(const LogType& level, const fsstring& message);
	SeekOrigin Int2SeekOrigin(const int32_t& value);

	//class FSID final : private noncopyable, private nonmovable
	//{
	//public:
	//	FSID();
	//	~FSID();
	//	int32_t getId();

	//private:
	//	int32_t id_ = 0;
	//};
}
