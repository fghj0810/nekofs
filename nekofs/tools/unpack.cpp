#include "unpack.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif
#include "../nekodata/nekodatafilesystem.h"
#include "../layer/layerfilesmeta.h"

#include <sstream>

namespace nekofs::tools {
	bool Unpack::exec(const std::string& filepath, const std::string& outpath)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		if (auto ft = nativefs->getFileType(filepath); ft != nekofs::FileType::Regular)
		{
			nekofs::logerr(u8"nekodata not found!");
			return false;
		}
		if (auto ft = nativefs->getFileType(outpath); ft != nekofs::FileType::None)
		{
			nekofs::logerr(u8"outpath already exist!");
			return false;
		}
		auto fs = NekodataFileSystem::createFromNative(filepath);
		nekofs::loginfo(u8"verify " + filepath + u8" ...");
		if (!fs || !fs->verify())
		{
			nekofs::logerr(u8"verify " + filepath + u8" ... failed");
			return false;
		}
		nekofs::loginfo(u8"verify " + filepath + u8" ... ok");
		auto lfm = nekofs::LayerFilesMeta::load(fs->openIStream(nekofs_kLayerFiles));
		if (lfm.has_value())
		{
			std::stringstream ss;
			ss << u8"unpack file ";
			ss << nekofs_kLayerVersion;
			loginfo(ss.str());
			if (!unpackOneFile(fs->openIStream(nekofs_kLayerVersion), outpath + nekofs_PathSeparator + nekofs_kLayerVersion))
			{
				logerr(u8"unpack error: " + outpath + nekofs_PathSeparator + nekofs_kLayerVersion);
				return false;
			}
			return unpackLayer(fs, &lfm.value(), outpath, "");
		}
		return unpackNormal(fs, outpath);
	}
	bool Unpack::unpackLayer(std::shared_ptr<FileSystem> fs, const LayerFilesMeta* lfm, const std::string& outpath, const std::string& progressInfo)
	{
		{
			std::stringstream ss;
			ss << u8"unpack file ";
			if (!progressInfo.empty())
			{
				ss << progressInfo << u8" < ";
			}
			ss << nekofs_kLayerFiles;
			loginfo(ss.str());
			if (!unpackOneFile(fs->openIStream(nekofs_kLayerFiles), outpath + nekofs_PathSeparator + nekofs_kLayerFiles))
			{
				logerr(u8"unpack error: " + outpath + nekofs_PathSeparator + nekofs_kLayerVersion);
				return false;
			}
		}

		auto allfiles = lfm->getFiles();
		size_t index = 0;
		size_t allfileNum = allfiles.size();
		for (const auto& item : allfiles)
		{
			index++;
			std::stringstream ss;
			ss << u8"unpack file ";
			if (!progressInfo.empty())
			{
				ss << progressInfo << u8" < ";
			}
			ss << u8"[" << index << u8"/" << allfileNum << u8"] ";
			ss << item.first;
			loginfo(ss.str());
			if (!unpackOneFile(fs->openIStream(item.first), outpath + nekofs_PathSeparator + item.first))
			{
				logerr(u8"unpack error: " + outpath + nekofs_PathSeparator + item.first);
				return false;
			}
		}

		auto allnekodatas = lfm->getNekodatas();
		for (const auto& item : allnekodatas)
		{
			auto layerfs = NekodataFileSystem::create(fs, item);
			if (!layerfs)
			{
				logerr(u8"unpackLayer error: open " + item + u8" failed!");
				return false;
			}
			auto lfm_tmp = nekofs::LayerFilesMeta::load(layerfs->openIStream(nekofs_kLayerFiles));
			if (!lfm_tmp.has_value())
			{
				logerr(u8"unpackLayer error: " + item + u8" open " + nekofs_kLayerFiles + u8" failed!");
				return false;
			}
			if (!unpackLayer(layerfs, &lfm_tmp.value(), outpath + nekofs_PathSeparator + item.substr(0, item.size() - nekofs_kNekodata_FileExtension.size()), progressInfo.empty() ? item : progressInfo + u8" < " + item))
			{
				logerr(u8"unpackLayer error: " + item);
				return false;
			}
		}
		return true;
	}
	bool Unpack::unpackNormal(std::shared_ptr<FileSystem> fs, const std::string& outpath)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		auto allfiles = fs->getAllFiles(u8"");
		for (size_t i = 0; i < allfiles.size(); i++)
		{
			const auto& item = allfiles[i];
			{
				std::stringstream ss;
				ss << u8"unpack file [" << i + 1 << u8"/" << allfiles.size() << u8"] ";
				ss << item;
				loginfo(ss.str());
			}
			if (!unpackOneFile(fs->openIStream(item), outpath + nekofs_PathSeparator + item))
			{
				logerr(u8"unpack error: " + outpath + nekofs_PathSeparator + item);
				return false;
			}
		}
		return true;
	}
	bool Unpack::unpackOneFile(std::shared_ptr<IStream> is, const std::string& outfilepath)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		auto buffer = env::getInstance().newBuffer4M();
		auto os = nativefs->openOStream(outfilepath);
		if (!os || !is)
		{
			return false;
		}
		int64_t totalLength = os->getLength();
		int32_t actualRead = 0;
		int32_t actualWrite = 0;
		do
		{
			actualWrite = 0;
			actualRead = istream_read(is, &(*buffer)[0], static_cast<int32_t>(buffer->size()));
			if (actualRead > 0)
			{
				actualWrite = ostream_write(os, &(*buffer)[0], actualRead);
			}
		} while (actualRead > 0 && actualRead == actualWrite);
		if (actualRead != 0 || actualRead != actualWrite)
		{
			return false;
		}
		return true;
	}
}
