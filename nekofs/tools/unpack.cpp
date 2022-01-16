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
		nekofs::loginfo(u8"verify nekodata ...");
		if (!fs || !fs->verify())
		{
			nekofs::logerr(u8"verify nekodata ... failed");
			return false;
		}
		nekofs::loginfo(u8"verify nekodata ... ok");
		auto allfiles = fs->getAllFiles(u8"");
		auto buffer = env::getInstance().newBuffer4M();
		for (size_t i = 0; i < allfiles.size(); i++)
		{
			const auto& item = allfiles[i];
			{
				std::stringstream ss;
				ss << u8"unpack file [" << i + 1 << u8"/" << allfiles.size() << u8"] ";
				ss << item;
				loginfo(ss.str());
			}
			auto is = fs->openIStream(item);
			auto os = nativefs->openOStream(outpath + nekofs_PathSeparator + item);
			if (!os || !is)
			{
				return false;
			}
			auto totalLength = os->getLength();
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
		}
		return true;
	}
}
