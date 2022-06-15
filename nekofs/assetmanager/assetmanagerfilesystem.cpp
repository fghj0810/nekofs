#include "assetmanagerfilesystem.h"
#include "assetmanagerfile.h"
#include "assetmanagerfilehandle.h"
#include "assetmanagerfileistream.h"
#include "../common/utils.h"

#include <android/asset_manager.h>
#include <cstring>
#include <sstream>
#include <functional>

namespace nekofs {
	std::string AssetManagerFileSystem::getCurrentPath() const
	{
		return std::string();
	}
	std::vector<std::string> AssetManagerFileSystem::getAllFiles(const std::string& dirpath) const
	{
		std::stringstream ss;
		ss << u8"AssetManagerFileSystem::getAllFiles unsupport. dirpath = ";
		ss << dirpath;
		logerr(ss.str());

		// 安卓没有提供递归遍历目录的接口
		return std::vector<std::string>();
	}
	std::unique_ptr<FileHandle> AssetManagerFileSystem::getFileHandle(const std::string& filepath)
	{
		return std::unique_ptr<FileHandle>(new AssetManagerFileHandle(openFileInternal(filepath)));
	}
	std::shared_ptr<IStream> AssetManagerFileSystem::openIStream(const std::string& filepath)
	{
		return openFileInternal(filepath)->openIStream();
	}
	FileType AssetManagerFileSystem::getFileType(const std::string& path) const
	{
		std::stringstream ss;
		ss << u8"AssetManagerFileSystem::getFileType poor performance, don't call this. path = ";
		ss << path;
		logwarn(ss.str());

		if (const_cast<AssetManagerFileSystem*>(this)->openIStream(path) != nullptr)
		{
			return FileType::Regular;
		}
		return FileType::Unkonwn;
	}
	int64_t AssetManagerFileSystem::getSize(const std::string& filepath) const
	{
		std::stringstream ss;
		ss << u8"AssetManagerFileSystem::getSize poor performance, don't call this. filepath = ";
		ss << filepath;
		logwarn(ss.str());

		auto is = const_cast<AssetManagerFileSystem*>(this)->openIStream(filepath);
		if (is != nullptr)
		{
			return is->getLength();
		}
		return -1;
	}
	FileSystemType AssetManagerFileSystem::getFSType() const
	{
		return FileSystemType::AssetManager;
	}


	std::vector<std::string> AssetManagerFileSystem::getFiles(const std::string& dirpath) const
	{
		auto amPtr = env::getAssetManagerPtr();
		if (amPtr == nullptr)
		{
			std::stringstream ss;
			ss << u8"AssetManagerFileSystem::getFiles amPtr is nullptr. dirpath = ";
			ss << dirpath;
			logerr(ss.str());
			return std::vector<std::string>();
		}
		std::vector<std::string> result;
		AAssetDir* d = ::AAssetManager_openDir(amPtr, dirpath.c_str());
		if (NULL != d)
		{
			for (const char* filename = ::AAssetDir_getNextFileName(d); NULL != filename; filename = ::AAssetDir_getNextFileName(d))
			{
				if (::strcmp(filename, ".") == 0 || ::strcmp(filename, "..") == 0)
				{
					continue;
				}
				result.push_back(filename);
			}
			::AAssetDir_close(d);
		}
		return result;
	}


	void AssetManagerFileSystem::weakDeleteCallback(std::weak_ptr<AssetManagerFileSystem> filesystem, AssetManagerFile* file)
	{
		auto fsPtr = filesystem.lock();
		if (fsPtr)
		{
			fsPtr->closeFileInternal(file->getFilePath());
		}
		else
		{
			delete file;
		}
	}
	std::shared_ptr<AssetManagerFile> AssetManagerFileSystem::openFileInternal(const std::string& filepath)
	{
		std::shared_ptr<AssetManagerFile> fPtr;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx_);
			auto it = files_.find(filepath);
			if (it != files_.end())
			{
				fPtr = it->second.lock();
			}
			if (!fPtr)
			{
				AssetManagerFile* rawPtr = nullptr;
				auto rit = filePtrs_.find(filepath);
				if (rit != filePtrs_.end())
				{
					rawPtr = rit->second;
				}
				else
				{
					rawPtr = new AssetManagerFile(filepath);
					filePtrs_[filepath] = rawPtr;
				}
				fPtr.reset(rawPtr, std::bind(&AssetManagerFileSystem::weakDeleteCallback, std::weak_ptr<AssetManagerFileSystem>(shared_from_this()), std::placeholders::_1));
				files_[filepath] = fPtr;
			}
		}
		return fPtr;
	}
	void AssetManagerFileSystem::closeFileInternal(const std::string& filepath)
	{
		std::shared_ptr<AssetManagerFile> fPtr;
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			fPtr = it->second.lock();
		}
		if (!fPtr)
		{
			if (it != files_.end())
			{
				files_.erase(it);
			}
			AssetManagerFile* rawPtr = nullptr;
			auto rit = filePtrs_.find(filepath);
			if (rit != filePtrs_.end())
			{
				std::swap(rawPtr, rit->second);
				filePtrs_.erase(rit);
				delete rawPtr;
			}
		}
	}
}
