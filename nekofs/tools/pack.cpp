#include "pack.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif
#include "../nekodata/nekodataarchiver.h"
#include "../layer/layerfilesmeta.h"

#include <sstream>

namespace nekofs::tools {
	bool Pack::exec(const std::string& dirpath, const std::string& outpath, int64_t volumeSize)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		if (auto ft = nativefs->getFileType(dirpath); ft != nekofs::FileType::Directory)
		{
			nekofs::logerr(u8"pack dir not found!");
			return false;
		}
		if (auto ft = nativefs->getFileType(outpath); ft != nekofs::FileType::None)
		{
			nekofs::logerr(u8"outpath already exist!");
			return false;
		}
		if (auto ft = nativefs->getFileType(dirpath + nekofs_PathSeparator + nekofs_kLayerVersion); ft != nekofs::FileType::Regular)
		{
			nekofs::logerr(u8"file not found! " + dirpath + nekofs_PathSeparator + nekofs_kLayerVersion);
			return false;
		}
		auto lfm = nekofs::LayerFilesMeta::load(nativefs->openIStream(dirpath + nekofs_PathSeparator + nekofs_kLayerFiles));
		if (!lfm.has_value())
		{
			nekofs::logerr(u8"open " + dirpath + nekofs_PathSeparator + nekofs_kLayerFiles + u8" failed!");
			return false;
		}
		auto archiver = std::make_shared<NekodataNativeArchiver>(outpath, volumeSize);
		archiver->addFile(nekofs_kLayerVersion, nativefs, dirpath + nekofs_PathSeparator + nekofs_kLayerVersion);
		archiver->addFile(nekofs_kLayerFiles, nativefs, dirpath + nekofs_PathSeparator + nekofs_kLayerFiles);
		auto allfiles = lfm->getFiles();
		for (const auto& item : allfiles)
		{
			archiver->addFile(item.first, nativefs, dirpath + nekofs_PathSeparator + item.first);
		}
		auto nekodatas = lfm->getNekodatas();
		for (const auto& item : nekodatas)
		{
			if (!packDir(archiver->addArchive(item), dirpath + nekofs_PathSeparator + item.substr(0, item.size() - nekofs_kNekodata_FileExtension.size())))
			{
				return false;
			}
		}
		return archiver->archive();
	}

	bool Pack::packDir(std::shared_ptr<nekofs::NekodataNativeArchiver> archiver, const std::string& dirpath)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		auto lfm = nekofs::LayerFilesMeta::load(nativefs->openIStream(dirpath + nekofs_PathSeparator + nekofs_kLayerFiles));
		if (!lfm.has_value())
		{
			nekofs::logerr(u8"open " + dirpath + nekofs_PathSeparator + nekofs_kLayerFiles + u8" failed!");
			return false;
		}
		archiver->addFile(nekofs_kLayerFiles, nativefs, dirpath + nekofs_PathSeparator + nekofs_kLayerFiles);
		auto allfiles = lfm->getFiles();
		for (const auto& item : allfiles)
		{
			archiver->addFile(item.first, nativefs, dirpath + nekofs_PathSeparator + item.first);
		}
		auto nekodatas = lfm->getNekodatas();
		for (const auto& item : nekodatas)
		{
			if (!packDir(archiver->addArchive(item), dirpath + nekofs_PathSeparator + item.substr(0, item.size() - nekofs_kNekodata_FileExtension.size())))
			{
				return false;
			}
		}
		return true;
	}
}
