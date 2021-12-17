#include "layerversionmeta.h"
#include "../common/error.h"
#include "../common/utils.h"

#include <algorithm>
#include <sstream>

namespace nekofs {
	std::optional<LayerVersionMeta> LayerVersionMeta::load(std::shared_ptr<IStream> is)
	{
		if (!is)
		{
			return std::nullopt;
		}
		JsonInputStream jis(is);
		JSONDocument d;
		d.ParseStream(jis);
		if (d.HasParseError())
		{
			return std::nullopt;
		}
		return load(&d);
	}
	std::optional<LayerVersionMeta> LayerVersionMeta::load(const JSONValue* jsondoc)
	{
		LayerVersionMeta lvm;
		{
			// fromVersion
			auto fromVersionIt = jsondoc->FindMember(nekofs_kLayerVersion_FromVersion);
			if (fromVersionIt != jsondoc->MemberEnd() && !fromVersionIt->value.IsNull())
			{
				lvm.from_version_ = fromVersionIt->value.GetUint();
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// version
			auto versionIt = jsondoc->FindMember(nekofs_kLayerVersion_Version);
			if (versionIt != jsondoc->MemberEnd() && !versionIt->value.IsNull())
			{
				lvm.version_ = versionIt->value.GetUint();
			}
			else
			{
				return std::nullopt;
			}
		}
		return lvm;
	}

	bool LayerVersionMeta::save(std::shared_ptr<OStream> os)
	{
		if (!os)
		{
			return false;
		}
		JsonOutputStream jos(os);
		JSONFileWriter writer(jos);
		JSONDocument d(rapidjson::kObjectType);
		if (!save(&d, d.GetAllocator()))
		{
			return false;
		}
		try
		{
			d.Accept(writer);
		}
		catch (const FSException& ex)
		{
			std::stringstream ss;
			ss << u8"LayerVersionMeta::save error. code = ";
			ss << (int32_t)ex.getErrCode();
			logerr(ss.str());
			return false;
		}
		return true;
	}
	bool LayerVersionMeta::save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator)
	{
		{
			// fromVersion
			JSONValue fromVersion(this->from_version_);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_FromVersion), fromVersion, allocator);
		}
		{
			// version
			JSONValue version(this->version_);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Version), version, allocator);
		}
		return true;
	}

	void LayerVersionMeta::setFromVersion(const uint32_t& from_version)
	{
		from_version_ = from_version;
	}
	uint32_t LayerVersionMeta::getFromVersion() const
	{
		return from_version_;
	}
	void LayerVersionMeta::setVersion(const uint32_t& version)
	{
		version_ = version;
	}
	uint32_t LayerVersionMeta::getVersion() const
	{
		return version_;
	}
}
