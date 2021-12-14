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
	class PrePareArgs final
	{
	public:
		static std::optional<PrePareArgs> load(const JSONValue* jsondoc);
		const std::string& getNativePath() const;

	private:
		std::string nativePath_;
	};

	class PrePare final
	{
	public:
		static bool exec(const PrePareArgs& arg);
	};
}

