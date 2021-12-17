#include "layerfilesystem.h"
#include "../common/env.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

#include <sstream>

namespace nekofs {
	std::shared_ptr<LayerFileSystem> LayerFileSystem::createNativeLayer(const std::string& dirpath)
	{
		auto lfs = std::make_shared<LayerFileSystem>();
		lfs->filesystem_ = env::getInstance().getNativeFileSystem();
		lfs->currentpath_ = dirpath;
		return lfs;
	}
	std::string LayerFileSystem::getFullPath(const std::string& path) const
	{
		if (path.empty())
		{
			return currentpath_;
		}
		return currentpath_ + nekofs_PathSeparator + path;
	}
	std::string LayerFileSystem::getFileURI(const std::string& path) const
	{
		std::stringstream ss;
		switch (filesystem_->getFSType())
		{
		case FileSystemType::Native:
			ss << nekofs_kURIPrefix_Native;
			break;
		default:
			return std::string();
		}
		ss << getFullPath(path);
		return ss.str();
	}


	std::string LayerFileSystem::getCurrentPath() const
	{
		return currentpath_;
	}
	std::vector<std::string> LayerFileSystem::getAllFiles(const std::string& dirpath) const
	{
		return filesystem_->getAllFiles(getFullPath(dirpath));
	}
	std::unique_ptr<FileHandle> LayerFileSystem::getFileHandle(const std::string& filepath)
	{
		return filesystem_->getFileHandle(getFullPath(filepath));
	}
	std::shared_ptr<IStream> LayerFileSystem::openIStream(const std::string& filepath)
	{
		return filesystem_->openIStream(getFullPath(filepath));
	}
	FileType LayerFileSystem::getFileType(const std::string& path) const
	{
		return filesystem_->getFileType(getFullPath(path));
	}
	int64_t LayerFileSystem::getSize(const std::string& filepath) const
	{
		return filesystem_->getSize(getFullPath(filepath));
	}
	FileSystemType LayerFileSystem::getFSType() const
	{
		return FileSystemType::Layer;
	}
}
