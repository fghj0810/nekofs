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
	class Pack final
	{
	public:
		static bool exec(const std::string& dirpath, const std::string& outpath, int64_t volumeSize);
	};
}

