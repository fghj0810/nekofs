﻿#include "nativefilesystem.h"
#include "nativefile.h"
#include "nativefilehandle.h"
#include "nativefileistream.h"
#include "nativefileostream.h"
#include "../common/utils.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <functional>
#include <queue>

namespace nekofs {
	std::string NativeFileSystem::getCurrentPath() const
	{
		return std::string();
	}
	std::vector<std::string> NativeFileSystem::getAllFiles(const std::string& dirpath) const
	{
		if (dirpath.empty())
		{
			return std::vector<std::string>();
		}
		std::vector<std::string> result;
		std::queue<std::string> dirs;
		dirs.push(dirpath);
		while (!dirs.empty())
		{
			auto currentpath = dirs.front();
			dirs.pop();
			DIR* d = ::opendir(currentpath.c_str());
			if (NULL == d)
			{
				continue;
			}
			for (struct dirent* dir = ::readdir(d); NULL != dir; dir = ::readdir(d))
			{
				if (::strcmp(dir->d_name, ".") == 0 || ::strcmp(dir->d_name, "..") == 0)
				{
					continue;
				}
				if (dir->d_type == DT_DIR)
				{
					dirs.emplace(currentpath + nekofs_PathSeparator + dir->d_name);
					continue;
				}
				else if (dir->d_type == DT_REG)
				{
					std::string path = currentpath + nekofs_PathSeparator + dir->d_name;
					result.push_back(path.substr(dirpath.size() + 1));
					continue;
				}
			}
			::closedir(d);
		}
		return result;
	}
	std::unique_ptr<FileHandle> NativeFileSystem::getFileHandle(const std::string& filepath)
	{
		return std::unique_ptr<FileHandle>(new NativeFileHandle(openFileInternal(filepath)));
	}
	std::shared_ptr<IStream> NativeFileSystem::openIStream(const std::string& filepath)
	{
		return openFileInternal(filepath)->openIStream();
	}
	bool NativeFileSystem::fileExist(const std::string& filepath) const
	{
		return getItemType(filepath) == ItemType::File;
	}
	bool NativeFileSystem::dirExist(const std::string& dirpath) const
	{
		return getItemType(dirpath) == ItemType::Directory;
	}
	int64_t NativeFileSystem::getSize(const std::string& filepath) const
	{
		struct stat info;
		if (0 == ::stat(filepath.c_str(), &info))
		{
			if ((info.st_mode & S_IFDIR) == 0 && (info.st_mode & S_IFREG) != 0)
			{
				return info.st_size;
			}
			else
			{
				std::stringstream ss;
				ss << "NativeFileSystem::getSize strange file. is dir? ";
				ss << filepath;
				logprint(LogType::Error, ss.str());
			}
		}
		else
		{
			std::stringstream ss;
			ss << "NativeFileSystem::getSize no such file! ";
			ss << filepath;
			logprint(LogType::Error, ss.str());
		}
		return -1;
	}
	FileSystemType NativeFileSystem::getFSType() const
	{
		return FileSystemType::Native;
	}


	bool NativeFileSystem::createDirectories(const std::string& dirpath)
	{
		std::vector<std::string> result;
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		for (size_t fpos = dirpath.size(); fpos != dirpath.npos; fpos = dirpath.rfind(nekofs_PathSeparator, fpos - 1))
		{
			std::string curpath = dirpath.substr(0, fpos);
			auto type = getItemType(curpath);
			if (type == ItemType::None) {
				result.push_back(curpath);
				if (std::filesystem::path tmp(curpath); tmp.parent_path() == tmp.root_path())
				{
					break;
				}
			}
			else if (type == ItemType::Directory)
			{
				break;
			}
			else
			{
				std::stringstream ss;
				ss << "NativeFileSystem::createDirectories error: path not a directory. dirpath = ";
				ss << dirpath;
				ss << ", curpath = ";
				ss << curpath;
				logprint(LogType::Error, ss.str());
				return false;
			}
		}
		for (size_t i = result.size(); i > 0; i--)
		{
			if (0 != ::mkdir(result[i - 1].c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << "NativeFileSystem::createDirectories mkdir error. dirpath = ";
				ss << result[i - 1];
				ss << ", err = ";
				ss << errmsg;
				logprint(LogType::Error, ss.str());
				return false;
			}
		}
		return true;
	}
	bool NativeFileSystem::removeDirectories(const std::string& dirpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		auto dirpath_type = getItemType(dirpath);
		if (dirpath_type == ItemType::None)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::removeDirectories dir not exist. path = ";
			ss << dirpath;
			logprint(LogType::Warning, ss.str());
			return true;
		}
		if (dirpath_type != ItemType::Directory)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::removeDirectories not a dir path. path = ";
			ss << dirpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (hasOpenFiles(dirpath))
		{
			std::stringstream ss;
			ss << "NativeFileSystem::removeDirectories unable to remove directories: some files in use. dirpath = ";
			ss << dirpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		const int OtherType = 0;
		const int FileType = 1;
		const int DirType = 2;
		struct stat info{};
		std::vector<std::pair<std::string, int>> result;
		std::queue<std::string> dirs;
		dirs.push(dirpath);
		result.push_back(std::pair<std::string, int>(dirpath, DirType));
		while (!dirs.empty())
		{
			std::string currentpath = dirs.front();
			dirs.pop();
			DIR* d = ::opendir(currentpath.c_str());
			if (NULL == d)
			{
				continue;
			}
			for (struct dirent* dir = ::readdir(d); NULL != dir; dir = ::readdir(d))
			{
				if (::strcmp(dir->d_name, ".") == 0 || ::strcmp(dir->d_name, "..") == 0)
				{
					continue;
				}
				if (dir->d_type == DT_DIR)
				{
					dirs.emplace(currentpath + nekofs_PathSeparator + dir->d_name);
					result.push_back(std::pair<std::string, int>(currentpath + nekofs_PathSeparator + dir->d_name, DirType));
					continue;
				}
				else if (dir->d_type == DT_REG)
				{
					result.push_back(std::pair<std::string, int>(currentpath + nekofs_PathSeparator + dir->d_name, FileType));
					continue;
				}
				else
				{
					result.push_back(std::pair<std::string, int>(currentpath + nekofs_PathSeparator + dir->d_name, OtherType));
					continue;
				}
			}
			::closedir(d);
		}
		bool findOther = false;
		for (const auto& item : result)
		{
			if (item.second == OtherType)
			{
				findOther = true;
				std::stringstream ss;
				ss << "NativeFileSystem::removeDirectories not dir or file. path = ";
				ss << item.first;
				logprint(LogType::Error, ss.str());
			}
		}
		if (!findOther)
		{
			for (const auto& item : result)
			{
				if (item.second == FileType)
				{
					if (-1 == ::unlink(item.first.c_str()))
					{
						auto errmsg = getSysErrMsg();
						std::stringstream ss;
						ss << "NativeFileSystem::removeDirectories unlink error. filepath = ";
						ss << item.first;
						ss << ", err = ";
						ss << errmsg;
						logprint(LogType::Error, ss.str());
						return false;
					}
				}
			}
			for (size_t i = result.size(); i > 0; i--)
			{
				const auto& item = result[i - 1];
				if (item.second == DirType)
				{
					if (-1 == ::rmdir(item.first.c_str()))
					{
						auto errmsg = getSysErrMsg();
						std::stringstream ss;
						ss << "NativeFileSystem::removeDirectories rmdir error. dirpath = ";
						ss << item.first;
						ss << ", err = ";
						ss << errmsg;
						logprint(LogType::Error, ss.str());
						return false;
					}
				}
			}
			return true;
		}
		return false;
	}
	bool NativeFileSystem::cleanEmptyDirectories(const std::string& dirpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		auto dirpath_type = getItemType(dirpath);
		if (dirpath_type == ItemType::None)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::cleanEmptyDirectories dir not exist. path = ";
			ss << dirpath;
			logprint(LogType::Warning, ss.str());
			return true;
		}
		if (dirpath_type != ItemType::Directory)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::cleanEmptyDirectories not a dir path. path = ";
			ss << dirpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		struct stat info{};
		std::vector<std::pair<std::string, int>> result;
		std::queue<std::string> dirs;
		dirs.push(dirpath);
		while (!dirs.empty())
		{
			std::string currentpath = dirs.front();
			result.push_back(std::pair<std::string, int>(currentpath, 0));
			auto& data = result.back();
			dirs.pop();
			DIR* d = ::opendir(currentpath.c_str());
			if (NULL == d)
			{
				continue;
			}
			for (struct dirent* dir = ::readdir(d); NULL != dir; dir = ::readdir(d))
			{
				if (::strcmp(dir->d_name, ".") == 0 || ::strcmp(dir->d_name, "..") == 0)
				{
					continue;
				}
				if (dir->d_type == DT_DIR)
				{
					dirs.emplace(currentpath + nekofs_PathSeparator + dir->d_name);
					continue;
				}
				else
				{
					data.second++;
					continue;
				}
			}
			::closedir(d);
		}
		for (size_t i = result.size(); i > 0; i--)
		{
			const auto& item = result[i - 1];
			if (item.second == 0)
			{
				if (-1 == ::rmdir(item.first.c_str()))
				{
					auto errmsg = getSysErrMsg();
					std::stringstream ss;
					ss << "NativeFileSystem::cleanEmptyDirectories rmdir error. dirpath = ";
					ss << item.first;
					ss << ", err = ";
					ss << errmsg;
					logprint(LogType::Error, ss.str());
					return false;
				}
			}
			else
			{
				auto parent = item.first.substr(0, item.first.rfind(nekofs_PathSeparator));
				for (size_t j = i - 1; j > 0; j--)
				{
					if (parent == result[j - 1].first)
					{
						result[j - 1].second += item.second;
						break;
					}
				}
			}
		}
		return true;
	}
	bool NativeFileSystem::moveDirectory(const std::string& srcpath, const std::string& destpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (!dirExist(srcpath))
		{
			std::stringstream ss;
			ss << "NativeFileSystem::moveDirectory dir not exist. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (hasOpenFiles(srcpath))
		{
			std::stringstream ss;
			ss << "NativeFileSystem::moveDirectory some files in use. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (getItemType(destpath) != ItemType::None)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::moveDirectory already exist. path = ";
			ss << destpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (-1 == ::rename(srcpath.c_str(), destpath.c_str()))
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << "NativeFileSystem::moveDirectory rename error. srcpath = ";
			ss << srcpath;
			ss << ", destpath = ";
			ss << destpath;
			ss << ", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
			return false;
		}
		return true;
	}
	bool NativeFileSystem::removeFile(const std::string& filepath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (filePtrs_.find(filepath) != filePtrs_.end())
		{
			std::stringstream ss;
			ss << "NativeFileSystem::removeFile file in use. path = ";
			ss << filepath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		auto filepath_type = getItemType(filepath);
		if (filepath_type == ItemType::None)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::removeFile file not exist. path = ";
			ss << filepath;
			logprint(LogType::Warning, ss.str());
			return true;
		}
		if (filepath_type != ItemType::File)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::removeFile not a file path. path = ";
			ss << filepath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (-1 == ::unlink(filepath.c_str()))
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << "NativeFileSystem::removeFile unlink error. filepath = ";
			ss << filepath;
			ss << ", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
			return false;
		}
		return true;
	}
	bool NativeFileSystem::moveFile(const std::string& srcpath, const std::string& destpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (filePtrs_.find(srcpath) != filePtrs_.end())
		{
			std::stringstream ss;
			ss << "NativeFileSystem::moveFile file in use. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (!fileExist(srcpath))
		{
			std::stringstream ss;
			ss << "NativeFileSystem::moveFile file not exist. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (getItemType(destpath) != ItemType::None)
		{
			std::stringstream ss;
			ss << "NativeFileSystem::moveFile already exist. path = ";
			ss << destpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (-1 == ::rename(srcpath.c_str(), destpath.c_str()))
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << "NativeFileSystem::moveFile rename error. srcpath = ";
			ss << srcpath;
			ss << ", destpath = ";
			ss << destpath;
			ss << ", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
			return false;
		}
		return true;
	}
	std::shared_ptr<OStream> NativeFileSystem::openOStream(const std::string& filepath)
	{
		return openFileInternal(filepath)->openOStream();
	}


	void NativeFileSystem::weakDeleteCallback(std::weak_ptr<NativeFileSystem> filesystem, NativeFile* file)
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
	std::shared_ptr<NativeFile> NativeFileSystem::openFileInternal(const std::string& filepath)
	{
		std::shared_ptr<NativeFile> fPtr;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx_);
			auto it = files_.find(filepath);
			if (it != files_.end())
			{
				fPtr = it->second.lock();
			}
			if (!fPtr)
			{
				NativeFile* rawPtr = nullptr;
				auto rit = filePtrs_.find(filepath);
				if (rit != filePtrs_.end())
				{
					rawPtr = rit->second;
				}
				else
				{
					rawPtr = new NativeFile(filepath);
					filePtrs_[filepath] = rawPtr;
				}
				fPtr.reset(rawPtr, std::bind(&NativeFileSystem::weakDeleteCallback, std::weak_ptr<NativeFileSystem>(shared_from_this()), std::placeholders::_1));
				files_[filepath] = fPtr;
			}
		}
		return fPtr;
	}
	void NativeFileSystem::closeFileInternal(const std::string& filepath)
	{
		std::shared_ptr<NativeFile> fPtr;
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
			NativeFile* rawPtr = nullptr;
			auto rit = filePtrs_.find(filepath);
			if (rit != filePtrs_.end())
			{
				filePtrs_.erase(rit);
				delete rawPtr;
			}
		}
	}
	bool NativeFileSystem::hasOpenFiles(const std::string& dirpath) const
	{
		std::string parentPath = dirpath + nekofs_PathSeparator;
		for (const auto& item : filePtrs_)
		{
			if (item.first.find(parentPath) == 0)
			{
				return true;
			}
		}
		return false;
	}
	ItemType NativeFileSystem::getItemType(const std::string& path) const
	{
		struct stat info;
		if (0 == ::stat(path.c_str(), &info))
		{
			if ((info.st_mode & S_IFDIR) != 0 && (info.st_mode & S_IFREG) == 0)
			{
				return ItemType::Directory;
			}
			else if ((info.st_mode & S_IFDIR) == 0 && (info.st_mode & S_IFREG) != 0)
			{
				return ItemType::File;
			}
			else
			{
				return ItemType::Unkonwn;
			}
		}
		return ItemType::None;
	}
}
