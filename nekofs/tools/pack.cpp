#include "pack.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif
#include "../nekodata/nekodatanativearchiver.h"

#include <sstream>

namespace nekofs::tools {
	bool Pack::exec(const std::string& dirpath, const std::string& outpath)
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
		auto allfiles = nativefs->getAllFiles(dirpath);
		auto os = nativefs->openOStream(outpath);
		if (!os)
		{
			nekofs::logerr(u8"create outputfile failed!");
			return false;
		}
		auto archiver = std::make_shared<NekodataNativeArchiver>();
		for (const auto& item : allfiles)
		{
			archiver->addFile(item, nativefs, dirpath + nekofs_PathSeparator + item);
		}
		return archiver->archive(os);
	}
}
