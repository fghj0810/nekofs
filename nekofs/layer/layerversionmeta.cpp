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
		if (jsondoc == nullptr || !jsondoc->IsObject())
		{
			return std::nullopt;
		}
		LayerVersionMeta lvm;
		{
			// name
			auto nameIt = jsondoc->FindMember(nekofs_kLayerVersion_Name);
			if (nameIt != jsondoc->MemberEnd() && nameIt->value.IsString())
			{
				lvm.setName(std::string(nameIt->value.GetString(), nameIt->value.GetStringLength()));
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// fromVersion
			auto fromVersionIt = jsondoc->FindMember(nekofs_kLayerVersion_FromVersion);
			if (fromVersionIt != jsondoc->MemberEnd() && fromVersionIt->value.IsUint())
			{
				lvm.fromVersion_ = fromVersionIt->value.GetUint();
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// version
			auto versionIt = jsondoc->FindMember(nekofs_kLayerVersion_Version);
			if (versionIt != jsondoc->MemberEnd() && versionIt->value.IsUint())
			{
				lvm.version_ = versionIt->value.GetUint();
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// branch
			auto branchIt = jsondoc->FindMember(nekofs_kLayerVersion_Branch);
			if (branchIt != jsondoc->MemberEnd() && branchIt->value.IsString())
			{
				lvm.branch_ = branchIt->value.GetString();
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// versionServers
			auto versionServersIt = jsondoc->FindMember(nekofs_kLayerVersion_VersionServers);
			if (versionServersIt != jsondoc->MemberEnd() && versionServersIt->value.IsArray())
			{
				for (auto it = versionServersIt->value.Begin(); it != versionServersIt->value.End(); it++)
				{
					if (it->IsString())
					{
						lvm.addVersionServer(std::string(it->GetString(), it->GetStringLength()));
					}
					else
					{
						return std::nullopt;
					}
				}
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// downloadServers
			auto downloadServersIt = jsondoc->FindMember(nekofs_kLayerVersion_DownloadServers);
			if (downloadServersIt != jsondoc->MemberEnd() && downloadServersIt->value.IsArray())
			{
				for (auto it = downloadServersIt->value.Begin(); it != downloadServersIt->value.End(); it++)
				{
					if (it->IsString())
					{
						lvm.addDownloadServer(std::string(it->GetString(), it->GetStringLength()));
					}
					else
					{
						return std::nullopt;
					}
				}
			}
			else
			{
				return std::nullopt;
			}
		}
		{
			// subResources
			auto subResourcesIt = jsondoc->FindMember(nekofs_kLayerVersion_SubResources);
			if (subResourcesIt != jsondoc->MemberEnd() && subResourcesIt->value.IsObject())
			{
				for (auto it = subResourcesIt->value.MemberBegin(); it != subResourcesIt->value.MemberEnd(); it++)
				{
					auto meta = LayerVersionMeta::SubResourceMeta::load(&(it->value));
					if (meta)
					{
						lvm.addSubResource(std::string(it->name.GetString(), it->name.GetStringLength()), meta.value());
					}
					else
					{
						return std::nullopt;
					}
				}
			}
		}
		{
			// relocate
			auto relocateIt = jsondoc->FindMember(nekofs_kLayerVersion_Relocate);
			if (relocateIt != jsondoc->MemberEnd() && relocateIt->value.IsString())
			{
				lvm.relocate_ = relocateIt->value.GetString();
			}
		}
		return lvm;
	}

	bool LayerVersionMeta::save(std::shared_ptr<OStream> os) const
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
			return d.Accept(writer);
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
	bool LayerVersionMeta::save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator) const
	{
		{
			// name
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Name), rapidjson::StringRef(this->name_), allocator);
		}
		{
			// fromVersion
			JSONValue fromVersion(this->fromVersion_);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_FromVersion), fromVersion, allocator);
		}
		{
			// version
			JSONValue version(this->version_);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Version), version, allocator);
		}
		{
			// branch
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Branch), rapidjson::StringRef(this->branch_), allocator);
		}
		{
			// versionServers
			JSONValue versionServers(rapidjson::kArrayType);
			for (const auto& item : this->versionServers_)
			{
				versionServers.PushBack(rapidjson::StringRef(item), allocator);
			}
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_VersionServers), versionServers, allocator);
		}
		{
			// downloadServers
			JSONValue downloadServers(rapidjson::kArrayType);
			for (const auto& item : this->downloadServers_)
			{
				downloadServers.PushBack(rapidjson::StringRef(item), allocator);
			}
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_DownloadServers), downloadServers, allocator);
		}
		{
			// subResources
			if (!this->subResources_.empty())
			{
				JSONValue subResources(rapidjson::kObjectType);
				for (const auto& item : this->subResources_)
				{
					JSONValue info(rapidjson::kObjectType);
					item.second.save(&info, allocator);
					subResources.AddMember(rapidjson::StringRef(item.first), info, allocator);
				}
				jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_SubResources), subResources, allocator);
			}
		}
		{
			// relocate
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Relocate), rapidjson::StringRef(this->relocate_), allocator);
		}
		return true;
	}

	void LayerVersionMeta::setName(const std::string& name)
	{
		name_ = name;
	}
	const std::string& LayerVersionMeta::getName() const
	{
		return name_;
	}
	void LayerVersionMeta::setFromVersion(const uint32_t& fromVersion)
	{
		fromVersion_ = fromVersion;
	}
	uint32_t LayerVersionMeta::getFromVersion() const
	{
		return fromVersion_;
	}
	void LayerVersionMeta::setVersion(const uint32_t& version)
	{
		version_ = version;
	}
	uint32_t LayerVersionMeta::getVersion() const
	{
		return version_;
	}
	void LayerVersionMeta::setBranch(const std::string& branch)
	{
		branch_ = branch;
	}
	const std::string& LayerVersionMeta::getBranch() const
	{
		return branch_;
	}
	void LayerVersionMeta::addVersionServer(const std::string& server)
	{
		versionServers_.push_back(server);
	}
	void LayerVersionMeta::cleanVersionServers()
	{
		versionServers_.clear();
	}
	const std::vector<std::string>& LayerVersionMeta::getVersionServers() const
	{
		return versionServers_;
	}
	void LayerVersionMeta::addDownloadServer(const std::string& server)
	{
		downloadServers_.push_back(server);
	}
	void LayerVersionMeta::cleanDownloadServers()
	{
		downloadServers_.clear();
	}
	const std::vector<std::string>& LayerVersionMeta::getDownloadServers() const
	{
		return downloadServers_;
	}
	void LayerVersionMeta::setRelocate(const std::string& relocate)
	{
		relocate_ = relocate;
	}
	const std::string& LayerVersionMeta::getRelocate() const
	{
		return relocate_;
	}
	void LayerVersionMeta::addSubResource(const std::string& name, const LayerVersionMeta::SubResourceMeta& meta)
	{
		subResources_[name] = meta;
	}
	void LayerVersionMeta::removeSubResource(const std::string& name)
	{
		auto it = subResources_.find(name);
		if (it != subResources_.end())
		{
			subResources_.erase(it);
		}
	}
	void LayerVersionMeta::cleanSubResources()
	{
		subResources_.clear();
	}
	std::optional<LayerVersionMeta::SubResourceMeta> LayerVersionMeta::getSubResource(const std::string& name) const
	{
		auto it = subResources_.find(name);
		if (it != subResources_.end())
		{
			return it->second;
		}
		return std::nullopt;
	}
	const std::map<std::string, LayerVersionMeta::SubResourceMeta>& LayerVersionMeta::getSubResources() const
	{
		return subResources_;
	}


	std::optional<LayerVersionMeta::SubResourceMeta> LayerVersionMeta::SubResourceMeta::load(const JSONValue* jsondoc)
	{
		if (jsondoc == nullptr || !jsondoc->IsObject())
		{
			return std::nullopt;
		}
		LayerVersionMeta::SubResourceMeta lvsrm;
		{
			// require
			auto relocateIt = jsondoc->FindMember(nekofs_kLayerVersion_Require);
			if (relocateIt != jsondoc->MemberEnd() && relocateIt->value.IsBool())
			{
				lvsrm.require_ = relocateIt->value.GetBool();
			}
			else
			{
				lvsrm.require_ = false;
			}
		}
		{
			// versionServers
			auto dependsIt = jsondoc->FindMember(nekofs_kLayerVersion_Depends);
			if (dependsIt != jsondoc->MemberEnd() && dependsIt->value.IsArray())
			{
				for (auto it = dependsIt->value.Begin(); it != dependsIt->value.End(); it++)
				{
					if (it->IsString())
					{
						lvsrm.addDepend(std::string(it->GetString(), it->GetStringLength()));
					}
					else
					{
						return std::nullopt;
					}
				}
			}
		}
		return lvsrm;
	}
	bool LayerVersionMeta::SubResourceMeta::save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator) const
	{
		{
			// require
			JSONValue version(this->require_);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Require), version, allocator);
		}
		{
			// depends
			JSONValue depends(rapidjson::kArrayType);
			for (const auto& item : this->depends_)
			{
				depends.PushBack(rapidjson::StringRef(item), allocator);
			}
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerVersion_Depends), depends, allocator);
		}
		return true;
	}
	void LayerVersionMeta::SubResourceMeta::setRequire(const bool& require)
	{
		require_ = require;
	}
	bool LayerVersionMeta::SubResourceMeta::getRequire() const
	{
		return require_;
	}
	void LayerVersionMeta::SubResourceMeta::addDepend(const std::string& branchName)
	{
		depends_.push_back(branchName);
	}
	void LayerVersionMeta::SubResourceMeta::cleanDepends()
	{
		depends_.clear();
	}
	const std::vector<std::string>& LayerVersionMeta::SubResourceMeta::getDepends() const
	{
		return depends_;
	}
}
