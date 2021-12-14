#include "layerfilesmeta.h"
#include "../common/error.h"
#include "../common/utils.h"
#include "../common/rapidjson.h"

#include <algorithm>
#include <sstream>

namespace nekofs {
	bool LayerFilesMeta::load(LayerFilesMeta& lfmeta, std::shared_ptr<IStream> is)
	{
		lfmeta = LayerFilesMeta();
		if (!is)
		{
			return false;
		}
		JsonInputStream jis(is);
		JSONDocument d;
		d.ParseStream(jis);
		if (d.HasParseError())
		{
			return false;
		}
		if (!load_internal(lfmeta, &d))
		{
			lfmeta = LayerFilesMeta();
			return false;
		}
		return true;
	}
	bool LayerFilesMeta::load(LayerFilesMeta& lfmeta, const JSONValue* jsondoc)
	{
		if (!load_internal(lfmeta, jsondoc))
		{
			lfmeta = LayerFilesMeta();
			return false;
		}
		return true;
	}
	bool LayerFilesMeta::save(const LayerFilesMeta& lfmeta, std::shared_ptr<OStream> os)
	{
		if (!os)
		{
			return false;
		}
		JsonOutputStream jos(os);
		JSONFileWriter writer(jos);
		JSONDocument d(rapidjson::kObjectType);
		if (!save(lfmeta, &d, d.GetAllocator()))
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
			ss << u8"LayerFilesMeta::save error. code = ";
			ss << (int32_t)ex.getErrCode();
			logprint(LogType::Error, ss.str());
			return false;
		}
		return true;
	}
	bool LayerFilesMeta::save(const LayerFilesMeta& lfmeta, JSONValue* jsondoc, JSONDocument::AllocatorType& allocator)
	{
		{
			// files
			JSONValue files(rapidjson::kObjectType);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerFiles_Files), files, allocator);
			for (const auto& item : lfmeta.files_)
			{
				JSONValue meta(rapidjson::kObjectType);
				JSONValue version(rapidjson::kNumberType);
				JSONValue sha256(rapidjson::kStringType);
				JSONValue size(rapidjson::kNumberType);
				version.SetUint(item.second.getVersion());
				std::string sha256str = sha256_to_str(item.second.getSHA256());
				sha256.SetString(sha256str.c_str(), allocator);
				size.SetInt64(item.second.getSize());
				meta.AddMember(rapidjson::StringRef(nekofs_kLayerFiles_FilesVersion), version, allocator);
				meta.AddMember(rapidjson::StringRef(nekofs_kLayerFiles_FilesSHA256), sha256, allocator);
				meta.AddMember(rapidjson::StringRef(nekofs_kLayerFiles_FilesSize), size, allocator);
				files.AddMember(rapidjson::StringRef(item.first.c_str()), meta, allocator);
			}
		}

		{
			// deletes
			JSONValue deletes(rapidjson::kObjectType);
			jsondoc->AddMember(rapidjson::StringRef(nekofs_kLayerFiles_Deletes), deletes, allocator);
			for (const auto& item : lfmeta.deletes_)
			{
				JSONValue version(rapidjson::kNumberType);
				version.SetUint(item.second);
				deletes.AddMember(rapidjson::StringRef(item.first.c_str()), version, allocator);
			}
		}
		return true;
	}

	void LayerFilesMeta::setFileMeta(const std::string& filename, const FileMeta& meta)
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
	void LayerFilesMeta::setDeleteVersion(const std::string& filename, const uint32_t& version)
	{
		auto it = files_.find(filename);
		if (it != files_.end())
		{
			files_.erase(it);
		}
		deletes_[filename] = version;
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

	bool LayerFilesMeta::load_internal(LayerFilesMeta& lfmeta, const JSONValue* jsondoc)
	{
		auto files = jsondoc->FindMember(nekofs_kLayerFiles_Files);
		if (files != jsondoc->MemberEnd() && !files->value.IsNull())
		{
			for (auto itr = files->value.MemberBegin(); itr != files->value.MemberEnd(); itr++)
			{
				std::string filename(itr->name.GetString());
				FileMeta meta;
				auto versionIt = itr->value.FindMember(nekofs_kLayerFiles_FilesVersion);
				if (versionIt != itr->value.MemberEnd())
				{
					meta.setVersion(versionIt->value.GetUint());
				}
				auto sha256strIt = itr->value.FindMember(nekofs_kLayerFiles_FilesSHA256);
				if (sha256strIt != itr->value.MemberEnd())
				{
					uint32_t v[8];
					str_to_sha256(sha256strIt->value.GetString(), v);
					meta.setSHA256(v);
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
			return false;
		}
		auto deletes = jsondoc->FindMember(nekofs_kLayerFiles_Deletes);
		if (deletes != jsondoc->MemberEnd() && !deletes->value.IsNull())
		{
			for (auto itr = deletes->value.MemberBegin(); itr != deletes->value.MemberEnd(); itr++)
			{
				std::string filename(itr->name.GetString());
				lfmeta.deletes_[filename] = itr->value.GetUint();
			}
		}
		else
		{
			return false;
		}
		return true;
	}


	void LayerFilesMeta::FileMeta::setVersion(const uint32_t& version)
	{
		version_ = version;
	}
	uint32_t LayerFilesMeta::FileMeta::getVersion() const
	{
		return version_;
	}
	void LayerFilesMeta::FileMeta::setSHA256(const uint32_t sha256[8])
	{
		std::copy(sha256, sha256 + 8, sha256_);
	}
	const uint32_t* LayerFilesMeta::FileMeta::getSHA256() const
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
