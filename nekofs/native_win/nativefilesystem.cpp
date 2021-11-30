﻿#include "nativefilesystem.h"
#include "nativefile.h"
#include "nativefilehandle.h"
#include "nativefileistream.h"
#include "nativefileostream.h"
#include "../common/utils.h"

#include <Windows.h>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <functional>
#include <queue>

namespace nekofs {
	fsstring NativeFileSystem::getCurrentPath() const
	{
		return fsstring();
	}
	std::vector<fsstring> NativeFileSystem::getAllFiles(const fsstring& dirpath) const
	{
		if (dirpath.empty())
		{
			return std::vector<fsstring>();
		}
		WIN32_FIND_DATA find_data{};
		std::vector<fsstring> result;
		std::queue<fsstring> dirs;
		dirs.push(dirpath);
		while (!dirs.empty())
		{
			auto currentpath = dirs.front();
			dirs.pop();
			HANDLE findHandle = FindFirstFile((dirpath + L"/*").c_str(), &find_data);
			if (INVALID_HANDLE_VALUE == findHandle)
			{
				continue;
			}
			do
			{
				if (::wcscmp(find_data.cFileName, L".") == 0 || ::wcscmp(find_data.cFileName, L"..") == 0)
				{
					continue;
				}
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					dirs.emplace(currentpath + L"/" + find_data.cFileName);
					continue;
				}
				else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
				{
					result.push_back(currentpath + L"/" + find_data.cFileName);
					continue;
				}
			} while (TRUE == FindNextFile(findHandle, &find_data));
			FindClose(findHandle);
		}
		return result;
	}
	std::unique_ptr<FileHandle> NativeFileSystem::getFileHandle(const fsstring& filepath)
	{
		return std::unique_ptr<FileHandle>(new NativeFileHandle(openFileInternal(filepath)));
	}
	std::shared_ptr<IStream> NativeFileSystem::openIStream(const fsstring& filepath)
	{
		return openFileInternal(filepath)->openIStream();
	}
	bool NativeFileSystem::fileExist(const fsstring& filepath) const
	{
		WIN32_FIND_DATA find_data;
		HANDLE findHandle = FindFirstFile(filepath.c_str(), &find_data);
		if (INVALID_HANDLE_VALUE != findHandle)
		{
			FindClose(findHandle);
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
			{
				return true;
			}
			else
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::fileExist not a file! ";
				ss << filepath;
				logprint(LogType::Error, ss.str());
			}
		}
		return false;
	}
	bool NativeFileSystem::dirExist(const fsstring& dirpath) const
	{
		WIN32_FIND_DATA find_data;
		HANDLE findHandle = FindFirstFile(dirpath.c_str(), &find_data);
		if (INVALID_HANDLE_VALUE != findHandle)
		{
			FindClose(findHandle);
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				return true;
			}
			else
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::dirExist not a directory! ";
				ss << dirpath;
				logprint(LogType::Error, ss.str());
			}
		}
		return false;
	}
	int64_t NativeFileSystem::getSize(const fsstring& filepath) const
	{
		WIN32_FIND_DATA find_data;
		HANDLE findHandle = FindFirstFile(filepath.c_str(), &find_data);
		if (INVALID_HANDLE_VALUE != findHandle)
		{
			FindClose(findHandle);
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
			{
				uint64_t hSize = find_data.nFileSizeHigh;
				uint64_t lSize = find_data.nFileSizeLow;
				return static_cast<int64_t>((hSize << 32) | lSize);
			}
			else
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::getSize strange file. is dir? ";
				ss << filepath;
				logprint(LogType::Error, ss.str());
			}
		}
		else
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::getSize no such file! ";
			ss << filepath;
			logprint(LogType::Error, ss.str());
		}
		return -1;
	}
	FileSystemType NativeFileSystem::getFSType() const
	{
		return FileSystemType::Native;
	}


	bool NativeFileSystem::createDirectories(const fsstring& dirpath)
	{
		WIN32_FIND_DATA find_data;
		HANDLE findHandle = FindFirstFile(dirpath.c_str(), &find_data);
		if (INVALID_HANDLE_VALUE != findHandle)
		{
			FindClose(findHandle);
			findHandle = INVALID_HANDLE_VALUE;
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				return true;
			}
			else
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::createDirectories filepath exist. not a directory. path = ";
				ss << dirpath;
				logprint(LogType::Error, ss.str());
			}
		}
		else
		{
			size_t pos = dirpath.rfind(L"/");
			if (pos != dirpath.npos)
			{
				if (createDirectories(dirpath.substr(0, pos)))
				{
					if (TRUE == CreateDirectory(dirpath.c_str(), NULL))
					{
						return true;
					}
					else
					{
						DWORD err = GetLastError();
						LPWSTR msgBuffer = NULL;
						if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
						{
							std::wstringstream ss;
							ss << L"NativeFileSystem::createDirectories CreateDirectory error. dirpath = ";
							ss << dirpath;
							ss << L", err = ";
							ss << msgBuffer;
							LocalFree(msgBuffer);
							logprint(LogType::Error, ss.str());
						};
						return false;
					}
				}
			}
		}
		return false;
	}
	bool NativeFileSystem::removeDirectories(const fsstring& dirpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (!dirExist(dirpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::removeDirectories dir not exist. path = ";
			ss << dirpath;
			logprint(LogType::Warning, ss.str());
			return true;
		}
		if (hasOpenFiles(dirpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::removeDirectories unable to remove directories: some files in use. dirpath = ";
			ss << dirpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		const int OtherType = 0;
		const int FileType = 1;
		const int DirType = 2;
		WIN32_FIND_DATA find_data{};
		std::vector<std::pair<fsstring, int>> result;
		std::queue<fsstring> dirs;
		dirs.push(dirpath);
		result.push_back(std::pair<fsstring, int>(dirpath, DirType));
		while (!dirs.empty())
		{
			fsstring currentpath = dirs.front();
			dirs.pop();
			HANDLE findHandle = FindFirstFile((dirpath + L"/*").c_str(), &find_data);
			if (INVALID_HANDLE_VALUE == findHandle)
			{
				continue;
			}
			do
			{
				if (::wcscmp(find_data.cFileName, L".") == 0 || ::wcscmp(find_data.cFileName, L"..") == 0)
				{
					continue;
				}
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					dirs.emplace(currentpath + L"/" + find_data.cFileName);
					result.push_back(std::pair<fsstring, int>(currentpath + L"/" + find_data.cFileName, DirType));
					continue;
				}
				else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
				{
					result.push_back(std::pair<fsstring, int>(currentpath + L"/" + find_data.cFileName, FileType));
					continue;
				}
				else
				{
					result.push_back(std::pair<fsstring, int>(currentpath + L"/" + find_data.cFileName, OtherType));
					continue;
				}
			} while (TRUE == FindNextFile(findHandle, &find_data));
			FindClose(findHandle);
		}
		bool findOther = false;
		for (const auto& item : result)
		{
			if (item.second == OtherType)
			{
				findOther = true;
				std::wstringstream ss;
				ss << L"NativeFileSystem::removeDirectories not dir or file. path = ";
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
					if (FALSE == DeleteFile(item.first.c_str()))
					{
						DWORD err = GetLastError();
						LPWSTR msgBuffer = NULL;
						if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
						{
							std::wstringstream ss;
							ss << L"NativeFileSystem::removeDirectories DeleteFile error. filepath = ";
							ss << item.first;
							ss << L", err = ";
							ss << msgBuffer;
							LocalFree(msgBuffer);
							logprint(LogType::Error, ss.str());
						};
						return false;
					}
				}
			}
			for (size_t i = result.size(); i > 0; i--)
			{
				const auto& item = result[i - 1];
				if (item.second == DirType)
				{
					if (FALSE == RemoveDirectory(item.first.c_str()))
					{
						DWORD err = GetLastError();
						LPWSTR msgBuffer = NULL;
						if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
						{
							std::wstringstream ss;
							ss << L"NativeFileSystem::removeDirectories RemoveDirectory error. dirpath = ";
							ss << item.first;
							ss << L", err = ";
							ss << msgBuffer;
							LocalFree(msgBuffer);
							logprint(LogType::Error, ss.str());
						};
						return false;
					}
				}
			}
			return true;
		}
		return false;
	}
	bool NativeFileSystem::cleanEmptyDirectories(const fsstring& dirpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (!dirExist(dirpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::cleanEmptyDirectories dir not exist. path = ";
			ss << dirpath;
			logprint(LogType::Warning, ss.str());
			return true;
		}
		WIN32_FIND_DATA find_data{};
		std::vector<std::pair<fsstring, int>> result;
		std::queue<fsstring> dirs;
		dirs.push(dirpath);
		while (!dirs.empty())
		{
			fsstring currentpath = dirs.front();
			result.push_back(std::pair<fsstring, int>(currentpath, 0));
			auto& data = result.back();
			dirs.pop();
			HANDLE findHandle = FindFirstFile((dirpath + L"/*").c_str(), &find_data);
			if (INVALID_HANDLE_VALUE == findHandle)
			{
				continue;
			}
			do
			{
				if (::wcscmp(find_data.cFileName, L".") == 0 || ::wcscmp(find_data.cFileName, L"..") == 0)
				{
					continue;
				}
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					continue;
				}
				else
				{
					data.second++;
					continue;
				}
			} while (TRUE == FindNextFile(findHandle, &find_data));
			FindClose(findHandle);
		}
		for (size_t i = result.size(); i > 0; i--)
		{
			const auto& item = result[i - 1];
			if (item.second == 0)
			{
				if (FALSE == RemoveDirectory(item.first.c_str()))
				{
					DWORD err = GetLastError();
					LPWSTR msgBuffer = NULL;
					if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
					{
						std::wstringstream ss;
						ss << L"NativeFileSystem::cleanEmptyDirectories RemoveDirectory error. dirpath = ";
						ss << item.first;
						ss << L", err = ";
						ss << msgBuffer;
						LocalFree(msgBuffer);
						logprint(LogType::Error, ss.str());
					};
					return false;
				}
			}
		}
		return true;
	}
	bool NativeFileSystem::moveDirectory(const fsstring& srcpath, const fsstring& destpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (hasOpenFiles(srcpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::moveDirectory some files in use. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (!dirExist(srcpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::moveDirectory dir not exist. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (itemExist(destpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::moveDirectory already exist. path = ";
			ss << destpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (FALSE == MoveFile(srcpath.c_str(), destpath.c_str()))
		{
			DWORD err = GetLastError();
			LPWSTR msgBuffer = NULL;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::moveDirectory MoveFileA error. srcpath = ";
				ss << srcpath;
				ss << L", destpath = ";
				ss << destpath;
				ss << L", err = ";
				ss << msgBuffer;
				LocalFree(msgBuffer);
				logprint(LogType::Error, ss.str());
			};
			return false;
		}
		return true;
	}
	bool NativeFileSystem::removeFile(const fsstring& filepath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (filePtrs_.find(filepath) != filePtrs_.end())
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::removeFile file in use. path = ";
			ss << filepath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (!fileExist(filepath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::removeFile file not exist. path = ";
			ss << filepath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (FALSE == DeleteFile(filepath.c_str()))
		{
			DWORD err = GetLastError();
			LPWSTR msgBuffer = NULL;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::removeFile DeleteFile error. filepath = ";
				ss << filepath;
				ss << L", err = ";
				ss << msgBuffer;
				LocalFree(msgBuffer);
				logprint(LogType::Error, ss.str());
			};
			return false;
		}
		return true;
	}
	bool NativeFileSystem::moveFile(const fsstring& srcpath, const fsstring& destpath)
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (filePtrs_.find(srcpath) != filePtrs_.end())
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::moveFile file in use. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (!fileExist(srcpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::moveFile file not exist. path = ";
			ss << srcpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (itemExist(destpath))
		{
			std::wstringstream ss;
			ss << L"NativeFileSystem::moveFile already exist. path = ";
			ss << destpath;
			logprint(LogType::Error, ss.str());
			return false;
		}
		if (FALSE == MoveFile(srcpath.c_str(), destpath.c_str()))
		{
			DWORD err = GetLastError();
			LPWSTR msgBuffer = NULL;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
			{
				std::wstringstream ss;
				ss << L"NativeFileSystem::moveFile MoveFileA error. srcpath = ";
				ss << srcpath;
				ss << L", destpath = ";
				ss << destpath;
				ss << L", err = ";
				ss << msgBuffer;
				LocalFree(msgBuffer);
				logprint(LogType::Error, ss.str());
			};
			return false;
		}
		return true;
	}
	std::shared_ptr<OStream> NativeFileSystem::openOStream(const fsstring& filepath)
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
	std::shared_ptr<NativeFile> NativeFileSystem::openFileInternal(const fsstring& filepath)
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
	void NativeFileSystem::closeFileInternal(const fsstring& filepath)
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
	bool NativeFileSystem::hasOpenFiles(const fsstring& dirpath) const
	{
		fsstring parentPath = dirpath + L"/";
		for (const auto& item : filePtrs_)
		{
			if (item.first.find(parentPath) == 0)
			{
				return true;
			}
		}
		return false;
	}
	bool NativeFileSystem::itemExist(const fsstring& path) const
	{
		WIN32_FIND_DATA find_data;
		HANDLE findHandle = FindFirstFile(path.c_str(), &find_data);
		if (INVALID_HANDLE_VALUE != findHandle)
		{
			FindClose(findHandle);
			return true;
		}
		return false;
	}
}
