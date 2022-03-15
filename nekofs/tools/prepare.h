#pragma once
#include "../common/typedef.h"

#include <cstdint>

namespace nekofs::tools {
	class PrePare final
	{
	public:
		static bool exec(const std::string& genpath, const std::string& versionfile, uint32_t versionOffset);

	private:
		static bool prepareDir(const std::string& genpath, uint32_t currentVersion);
	};
}

