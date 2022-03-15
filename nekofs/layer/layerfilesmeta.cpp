#include "layerfilesmeta.h"
#include "../common/error.h"
#include "../common/utils.h"

#include <algorithm>
#include <sstream>

namespace nekofs {
	std::optional<LayerFilesMeta> LayerFilesMeta::load(std::shared_ptr<IStream> is)
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
	std::optional<LayerFilesMeta> LayerFilesMeta::load(const JSONValue* jsondoc)
	{
		if (jsondoc == nullptr || !jsondoc->IsObject())
		{
			return std::nullopt;
		}
		LayerFilesMeta lfmeta;
		{
			// files
			auto files = jsondoc->FindMember(nekofs_kLayerFiles_Files);
			if (files != jsondoc->MemberEnd() && !files->value.IsNull())
			{
				for (auto itr = files->value.MemberBegin(); itr != files->value.MemberEnd(); itr++)
				{
					std::string filename(itr->name.GetString(), itr->name.GetStringLength());
					LayerFilesMeta::FileMeta meta;
					auto versionIt = itr->value.FindMember(nekofs_kLayerFiles_FilesVersion);
					if (versionIt != itr->value.MemberEnd())
					{
						meta.setVersion(versionIt->value.GetUint());
					}
					auto sha256strIt = itr->value.FindMember(nekofs_kLayerFiles_FilesSHA256);
					if (sha256strIt != itr->value.MemberEnd())
					{
						meta.setSHA256(str_to_sha256(sha256strIt->value.GetString()));
					}
					auto sizeIt = itr->value.FindMember(nekofs_kLayerFiles_FilesSize);
					if (sizeIt != itr->value.MemberEnd())
					{
						meta.setSize(sizeIt->value.GetInt64());
					}
					lfmeta.files_[filename] = meta;
				}
			}
			else
			{
				return std::nullopt;
			}
		}
		// nekodatas
		{
			auto deletes = jsondoc->FindMember(nekofs_kLayerFiles_Nekodatas);
			if (deletes != jsondoc->MemberEnd() && deletes->value.IsArray())
			{
				for (auto itr = deletes->value.Begin(); itr != deletes->value.End(); itr++)
				{
					std::string filename(itr->GetString(), itr->GetStringLength());
					lfmeta.nekodatas_.insert(filename);
				}
			}
			else
			{
				return std::nullopt;
			}
		}
		// deletes
		{
			auto deletes = jsondoc->FindMember(nekofs_kLayerFiles_Deletes);
			if (deletes != jsondoc->MemberEnd() && !deletes->value.IsNull())
			{
				for (auto itr = deletes->value.MemberBegin(); itr != deletes->value.MemberEnd(); itr++)
				{
					std::string filename(itr->name.GetString(), itr->name.GetStringLength());
					lfmeta.deletes_[filename] = itr->value.GetUint();
				}
			}
			else
			{
				return std::nullopt;
			}
		}
		return lfmeta;
	}
	LayerFilesMeta LayerFilesMeta::merge(const std::vector<LayerFilesMeta>& lfms, uint32_t baseVersion)
	{
		LayerFilesMeta lfm;
		if (!lfms.empty())
		{
			lfm = lfms[0];
			for (size_t i = 1; i < lfms.size(); i++)
			{
				for (const auto& item : lfms[i].files_)
				{
					lfm.setFileMeta(item.first, item.second);
				}
				for (const auto& item : lfms[i].deletes_)
				{
					lfm.setDeleteVersion(item.first, item.second);
				}
				for (const auto& item : lfms[i].nekodatas_)
				{
					lfm.addNekodata(item);
				}
			}
		}
		for (auto it = lfm.files_.cbegin(); it != lfm.files_.cend();)
		{
			if (it->second.getVersion() < baseVersion)
			{
				it = lfm.files_.erase(it);
			}
			else
			{
				it++;
			}
		}
		for (auto it = lfm.deletes_.cbegin(); it != lfm.deletes_.cend();)
		{
			if (it->second < baseVersion)
			{
				it = lfm.deletes_.erase(it);
			}
			else
			{
				it++;
			}
		}
		return lfm;
	}
	LayerFilesMeta LayerFilesMeta::makediff(const LayerFilesMeta& lfms_earlier, const LayerFilesMeta& lfms_latest, const uint32_t latestVersion)
	{
		LayerFilesMeta lfm = lfms_latest;
		for (const auto& item : lfms_earlier.nekodatas_)
		{
			lfm.addNekodata(item);
		}
		for (const auto& item : lfms_earlier.files_)
		{
			auto meta_latest = lfms_latest.getFileMeta(item.first);
			if (!meta_latest.has_value())
			{
				lfm.setDeleteVersion(item.first, latestVersion);
			}
			else if (meta_latest->getSHA256() == item.second.getSHA256() && meta_latest->getSize() == item.second.getSize())
			{
				lfm.files_.erase(item.first);
			}
			else
			{
				lfm.files_[item.first].setVersion(std::max(meta_latest->getVersion(), item.second.getVersion()));
			}
		}
		for (const auto& item : lfms_earlier.deletes_)
		{
			if (lfms_latest.files_.find(item.first) != lfms_latest.files_.end())
			{
				lfm.deletes_[item.first] = std::max(item.second, lfm.deletes_[item.first]);
			}
		}
		return lfm;
	}
	bool LayerFilesMeta::save(std::shared_ptr<OStream> os) const
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
			ss << u8"LayerFilesMeta::save error. code = ";
			ss << (int32_t)ex.getErrCode();
			logerr(ss.str());
			return false;
		}
		return true;
	}
	bool LayerFilesMeta::save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator) const
	{
		{
			// files
			JSONValue files(rapidjson::kObjectType);
			for (const auto& item : this->files_)
			{
				std::string sha256str = sha256_to_str(item.second.getSHA256());
				JSONValue meta(rapidjson::kObjectType);
				JSONValue version(item.second.getVersion());
				JSONValue sha256(sha256str, allocator);
				JSONValue size(item.second.getSize());
				meta.AddMember(rapidjson::StringRef(nekofs_kLayerFiles_FilesVersion), version, allocator);
				meta.AddMember(rapidjson::StringRef(nekofs_kLayerFiles_FilesSHA256), sha256, allocator);
				meta.AddMember(rapidjson::StringRef(nekofs_kLayerFiles_FilesSize), size, allocator);
				files.AddMember(rapidjson::StringRef(item.first.c_str()), meta, allocator);
			}
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerFiles_Files), files, allocator);
		}
		{
			// nekodatas
			JSONValue nekodatas(rapidjson::kArrayType);
			for (const auto& item : this->nekodatas_)
			{
				nekodatas.PushBack(rapidjson::StringRef(item), allocator);
			}
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerFiles_Nekodatas), nekodatas, allocator);
		}
		{
			// deletes
			JSONValue deletes(rapidjson::kObjectType);
			for (const auto& item : this->deletes_)
			{
				JSONValue version(item.second);
				deletes.AddMember(rapidjson::StringRef(item.first), version, allocator);
			}
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerFiles_Deletes), deletes, allocator);
		}
		return true;
	}

	void LayerFilesMeta::setFileMeta(const std::string& filename, const LayerFilesMeta::FileMeta& meta)
	{
		auto it = deletes_.find(filename);
		if (it != deletes_.end())
		{
			deletes_.erase(it);
		}
		files_[filename] = meta;
	}
	std::optional<LayerFilesMeta::FileMeta> LayerFilesMeta::getFileMeta(const std::string& filename) const
	{
		auto it = files_.find(filename);
		if (it != files_.end())
		{
			return it->second;
		}
		return std::nullopt;
	}
	const std::map<std::string, LayerFilesMeta::FileMeta>& LayerFilesMeta::getFiles() const
	{
		return files_;
	}
	void LayerFilesMeta::addNekodata(const std::string& filename)
	{
		nekodatas_.insert(filename);
	}
	const std::set<std::string>& LayerFilesMeta::getNekodatas() const
	{
		return nekodatas_;
	}
	void LayerFilesMeta::setDeleteVersion(const std::string& filename, const uint32_t& version)
	{
		auto it = files_.find(filename);
		if (it != files_.end())
		{
			files_.erase(it);
		}
		// 设置version为0 代表删除
		if (version > 0)
		{
			deletes_[filename] = version;
		}
	}
	std::optional<uint32_t> LayerFilesMeta::getDeleteVersion(const std::string& filename) const
	{
		auto it = deletes_.find(filename);
		if (it != deletes_.end())
		{
			return it->second;
		}
		return std::nullopt;
	}
	const std::map<std::string, uint32_t>& LayerFilesMeta::getDeletes() const
	{
		return deletes_;
	}


	void LayerFilesMeta::FileMeta::setVersion(const uint32_t& version)
	{
		version_ = version;
	}
	uint32_t LayerFilesMeta::FileMeta::getVersion() const
	{
		return version_;
	}
	void LayerFilesMeta::FileMeta::setSHA256(const std::array<uint32_t, 8>& sha256)
	{
		sha256_ = sha256;
	}
	const std::array<uint32_t, 8>& LayerFilesMeta::FileMeta::getSHA256() const
	{
		return sha256_;
	}
	void LayerFilesMeta::FileMeta::setSize(const int64_t& size)
	{
		size_ = size;
	}
	int64_t LayerFilesMeta::FileMeta::getSize() const
	{
		return size_;
	}
}
