#include "prepare.h"
#include "../layer/layerversionmeta.h"
#include "../layer/layerfilesmeta.h"
#include "../layer/layerfilesystem.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

namespace nekofs::tools {
	bool PrePare::exec(const std::string& genpath, const std::string& versionfile, uint32_t versionOffset)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		if (auto ft = nativefs->getFileType(genpath + nekofs_PathSeparator + nekofs_kLayerFiles); ft != nekofs::FileType::None)
		{
			if (nativefs->removeFile(genpath + nekofs_PathSeparator + nekofs_kLayerFiles))
			{
				return false;
			}
		}
		auto lvm = nekofs::LayerVersionMeta::load(nativefs->openIStream(versionfile));
		if (!lvm.has_value())
		{
			nekofs::logerr(u8"load version faild!");
			return false;
		}
		lvm->setVersion(lvm->getVersion() + versionOffset);
		if (lvm->getVersion() == 0)
		{
			nekofs::logerr(u8"lvm->getVersion() == 0");
			return false;
		}
		if (auto ft = nativefs->getFileType(genpath + nekofs_PathSeparator + nekofs_kLayerVersion); ft != nekofs::FileType::None)
		{
			if (!nativefs->removeFile(genpath + nekofs_PathSeparator + nekofs_kLayerVersion))
			{
				return false;
			}
		}

		const uint32_t buffer_size = 8 * 1024;
		char buffer[8 * 1024];
		nekofs::LayerFilesMeta lfm;
		auto allfiles = nativefs->getAllFiles(genpath);
		for (const auto& item : allfiles)
		{
			sha256sum sum;
			auto is = nativefs->openIStream(genpath + nekofs_PathSeparator + item);
			int32_t actualRead = 0;
			do
			{
				actualRead = is->read(buffer, buffer_size);
				if (actualRead > 0)
				{
					sum.update(buffer, actualRead);
				}
			} while (actualRead > 0);
			if (actualRead < 0)
			{
				return false;
			}
			sum.final();
			uint32_t hash[8];
			sum.readHash(hash);
			nekofs::LayerFilesMeta::FileMeta meta;
			meta.setVersion(lvm->getVersion());
			meta.setSHA256(hash);
			meta.setSize(is->getLength());
			lfm.setFileMeta(item, meta);
		}
		auto fos = nativefs->openOStream(genpath + nekofs_PathSeparator + nekofs_kLayerFiles);
		auto vos = nativefs->openOStream(genpath + nekofs_PathSeparator + nekofs_kLayerVersion);
		return lfm.save(fos) && lvm->save(vos);
	}
}
