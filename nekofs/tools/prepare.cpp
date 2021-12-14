#include "prepare.h"

namespace nekofs {
	constexpr const char* prepare_nativepath = u8"nativepath";

	std::optional<PrePareArgs> PrePareArgs::load(const JSONValue* jsondoc)
	{
		//auto files = jsondoc->FindMember(nekofs_kLayerFiles_Files);
		//if (files != jsondoc->MemberEnd() && !files->value.IsNull())
		//{
		//	for (auto itr = files->value.MemberBegin(); itr != files->value.MemberEnd(); itr++)
		//	{
		//		std::string filename(itr->name.GetString());
		//		FileMeta meta;
		//		auto versionIt = itr->value.FindMember(nekofs_kLayerFiles_FilesVersion);
		//		if (versionIt != itr->value.MemberEnd())
		//		{
		//			meta.setVersion(versionIt->value.GetUint());
		//		}
		//		auto sha256strIt = itr->value.FindMember(nekofs_kLayerFiles_FilesSHA256);
		//		if (sha256strIt != itr->value.MemberEnd())
		//		{
		//			uint32_t v[8];
		//			str_to_sha256(sha256strIt->value.GetString(), v);
		//			meta.setSHA256(v);
		//		}
		//		auto sizeIt = itr->value.FindMember(nekofs_kLayerFiles_FilesSize);
		//		if (sizeIt != itr->value.MemberEnd())
		//		{
		//			meta.setSize(sizeIt->value.GetInt64());
		//		}
		//		lfmeta.files_[filename] = meta;
		//	}
		//}
		//else
		//{
		//	return false;
		//}
		//auto deletes = jsondoc->FindMember(nekofs_kLayerFiles_Deletes);
		//if (deletes != jsondoc->MemberEnd() && !deletes->value.IsNull())
		//{
		//	for (auto itr = deletes->value.MemberBegin(); itr != deletes->value.MemberEnd(); itr++)
		//	{
		//		std::string filename(itr->name.GetString());
		//		lfmeta.deletes_[filename] = itr->value.GetUint();
		//	}
		//}
		//else
		//{
		//	return false;
		//}
		//return true;
		return std::optional<PrePareArgs>();
	}
}
