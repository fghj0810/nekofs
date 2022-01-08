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
	class Unpack final
	{
	public:
		static bool exec(const std::string& filepath, const std::string& outpath);
	};
}

