#pragma once
#include "../common/typedef.h"

#include <cstdint>
#include <vector>

namespace nekofs::tools {
	class Merge final
	{
	public:
		static bool execNekodata(const std::string& outfilepath, int64_t volumeSize, const std::vector<std::string> patchfiles, bool verify = true);
		static bool execDir(const std::string& outdirpath, int64_t volumeSize, const std::vector<std::string> patchfiles, bool verify = true);
	};
}

