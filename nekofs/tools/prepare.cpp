#include "prepare.h"
#include "../layer/layerfilesmeta.h"
#include "../layer/layerfilesystem.h"
#include "../common/env.h"
#include "../common/sha256.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

namespace nekofs::tools {
	bool PrePare::exec(const std::string& genpath)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		if (nativefs->fileExist(genpath + nekofs_PathSeparator + nekofs_kLayerFiles))
		{
			nativefs->removeFile(genpath + nekofs_PathSeparator + nekofs_kLayerFiles);
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
			meta.setVersion(0);
			meta.setSHA256(hash);
			meta.setSize(is->getLength());
			lfm.setFileMeta(item, meta);
		}
		auto fos = nativefs->openOStream(genpath + nekofs_PathSeparator + nekofs_kLayerFiles);
		return nekofs::LayerFilesMeta::save(lfm, fos);
	}
}
