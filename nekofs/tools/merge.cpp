#include "merge.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#include "../layer/layerfilesystem.h"
#include "../layer/layerfilesmeta.h"
#include "../layer/layerversionmeta.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif
#include "../nekodata/nekodatafilesystem.h"
#include "../nekodata/nekodataarchiver.h"
#include "../update/merger.h"

#include <sstream>

namespace nekofs::tools {
	static inline std::shared_ptr<Merger> prepare(const std::vector<std::string> patchfiles, bool verify)
	{
		// 对第一个包特殊处理，因为要读取resName和baseVersion
		auto fs_firstPatchPath = nekofs::NekodataFileSystem::createFromNative(patchfiles[0]);
		if (!fs_firstPatchPath)
		{
			nekofs::logerr(u8"open " + patchfiles[0] + u8" ... failed!");
			return nullptr;
		}
		if (verify)
		{
			nekofs::loginfo(u8"verify " + patchfiles[0] + u8" ...");
			if (!fs_firstPatchPath->verify())
			{
				nekofs::logerr(u8"verify " + patchfiles[0] + u8" ... failed!");
				return nullptr;
			}
			nekofs::loginfo(u8"verify " + patchfiles[0] + u8" ... ok!");
		}
		auto lvm = nekofs::LayerVersionMeta::load(fs_firstPatchPath->openIStream(nekofs_kLayerVersion));
		if (!lvm.has_value())
		{
			return nullptr;
		}
		auto merger = std::make_shared<Merger>(lvm->getName(), lvm->getFromVersion());
		if (!merger->addPatch(fs_firstPatchPath))
		{
			return nullptr;
		}

		for (size_t i = 1; i < patchfiles.size(); i++)
		{
			auto fs = nekofs::NekodataFileSystem::createFromNative(patchfiles[i]);
			if (!fs)
			{
				nekofs::logerr(u8"open " + patchfiles[i] + u8" ... failed!");
				return nullptr;
			}
			if (verify)
			{
				nekofs::loginfo(u8"verify " + patchfiles[i] + u8" ...");
				if (!fs->verify())
				{
					nekofs::logerr(u8"verify " + patchfiles[i] + u8" ... failed!");
					return nullptr;
				}
				nekofs::loginfo(u8"verify " + patchfiles[i] + u8" ... ok!");
			}
			if (!merger->addPatch(fs))
			{
				return nullptr;
			}
		}
		return merger;
	}

	bool Merge::execNekodata(const std::string& outfilepath, int64_t volumeSize, const std::vector<std::string> patchfiles, bool verify)
	{
		auto merger = prepare(patchfiles, verify);
		if (merger)
		{
			auto archiver = std::make_shared<NekodataArchiver>(outfilepath, volumeSize);
			return merger->exec(archiver);
		}
		return false;
	}
	bool Merge::execDir(const std::string& outdirpath, int64_t volumeSize, const std::vector<std::string> patchfiles, bool verify)
	{
		auto merger = prepare(patchfiles, verify);
		if (merger)
		{
			return merger->exec(outdirpath, volumeSize);
		}
		return false;
	}
}
