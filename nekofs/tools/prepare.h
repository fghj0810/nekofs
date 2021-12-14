#pragma once
#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"
#include "../common/rapidjson.h"

#include <cstdint>
#include <memory>
#include <map>
#include <optional>

namespace nekofs {
	class PrePareArgs final
	{
	public:
		static std::optional<PrePareArgs> load(const JSONValue* jsondoc);

	private:
		std::string nativePath_;
	};
}

