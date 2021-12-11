#include "layerfilesystem.h"
#include "../common/env.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

namespace nekofs {
	std::shared_ptr<LayerFileSystem> LayerFileSystem::createNativeLayer(const fsstring& dirpath)
	{
		auto lfs = std::make_shared<LayerFileSystem>();
		lfs->filesystem_ = env::getInstance().getNativeFileSystem();
		lfs->currentpath_ = dirpath;
		return lfs;
	}
	fsstring LayerFileSystem::getFullPath(const fsstring& path) const
	{
		return currentpath_ + nekofs_PathSeparator + path;
	}


	fsstring LayerFileSystem::getCurrentPath() const
	{
		return currentpath_;
	}
	std::vector<fsstring> LayerFileSystem::getAllFiles(const fsstring& dirpath) const
	{
		return filesystem_->getAllFiles(getFullPath(dirpath));
	}
	std::unique_ptr<FileHandle> LayerFileSystem::getFileHandle(const fsstring& filepath)
	{
		return filesystem_->getFileHandle(getFullPath(filepath));
	}
	std::shared_ptr<IStream> LayerFileSystem::openIStream(const fsstring& filepath)
	{
		return filesystem_->openIStream(getFullPath(filepath));
	}
	bool LayerFileSystem::fileExist(const fsstring& filepath) const
	{
		return filesystem_->fileExist(getFullPath(filepath));
	}
	bool LayerFileSystem::dirExist(const fsstring& dirpath) const
	{
		return filesystem_->dirExist(getFullPath(dirpath));
	}
	int64_t LayerFileSystem::getSize(const fsstring& filepath) const
	{
		return filesystem_->getSize(getFullPath(filepath));
	}
	FileSystemType LayerFileSystem::getFSType() const
	{
		return FileSystemType::Layer;
	}
}
