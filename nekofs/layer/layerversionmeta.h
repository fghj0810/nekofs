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
	class LayerVersionMeta final
	{
	public:
		static std::optional<LayerVersionMeta> load(std::shared_ptr<IStream> is);
		static std::optional<LayerVersionMeta> load(const JSONValue* jsondoc);
		bool save(std::shared_ptr<OStream> os);
		bool save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator);
		void setFromVersion(const uint32_t& from_version);
		uint32_t getFromVersion() const;
		void setVersion(const uint32_t& version);
		uint32_t getVersion() const;

	private:
		uint32_t from_version_ = 0;
		uint32_t version_ = 0;
	};
}
