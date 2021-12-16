#pragma once
#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"
#include "../common/rapidjson.h"

#include <cstdint>
#include <memory>
#include <map>
#include <optional>

namespace nekofs::tools {
	class PrePare final
	{
	public:
		static bool exec(const std::string& genpath, const std::string& versionfile, uint32_t versionOffset);
	};
}

