#pragma once

#include "../common/typedef.h"
#include <Windows.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <map>

namespace nekofs {
	class NativeIStream;
	class NativeOStream;
	class NativeFile;

	class NativeFileSystem final : public FileSystem, public std::enable_shared_from_this<NativeFileSystem>
	{
	private:
		NativeFileSystem(const NativeFileSystem&) = delete;
		NativeFileSystem(NativeFileSystem&&) = delete;
		NativeFileSystem& operator=(const NativeFileSystem&) = delete;
		NativeFileSystem& operator=(NativeFileSystem&&) = delete;

	public:
		std::string getCurrentPath() const override;
		std::vector<std::string> getAllFiles(const std::string& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) override;
		std::shared_ptr<IStream> openIStream(const std::string& filepath) override;
		FileType getFileType(const std::string& path) const override;
		int64_t getSize(const std::string& filepath) const override;
		FileSystemType getFSType() const override;

	public:
		NativeFileSystem() = default;
		std::vector<std::string> getFiles(const std::string& dirpath) const;
		std::vector<std::string> getDirs(const std::string& dirpath) const;
		bool createDirectories(const std::string& dirpath);
		bool removeDirectories(const std::string& dirpath);
		bool cleanEmptyDirectories(const std::string& dirpath);
		bool moveDirectory(const std::string& srcpath, const std::string& destpath);
		bool removeFile(const std::string& filepath);
		bool moveFile(const std::string& srcpath, const std::string& destpath);
		std::shared_ptr<OStream> openOStream(const std::string& filepath);

	private:
		static void weakDeleteCallback(std::weak_ptr<NativeFileSystem> filesystem, NativeFile* file);
		std::shared_ptr<NativeFile> openFileInternal(const std::string& filepath);
		void closeFileInternal(const std::string& filepath);
		bool hasOpenFiles(const std::string& dirpath) const;

	private:
		std::map<std::string, std::weak_ptr<NativeFile>> files_;
		std::map<std::string, NativeFile*> filePtrs_;
		std::recursive_mutex mtx_;
	};


	inline std::wstring u8_to_u16(const std::string& u8str)
	{
		if (!u8str.empty())
		{
			auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &u8str[0], (int)u8str.size(), NULL, 0);
			std::wstring u16str(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, &u8str[0], (int)u8str.size(), &u16str[0], size_needed);
			return u16str;
		}
		return std::wstring();
	}
	inline std::string u16_to_u8(const std::wstring& u16str)
	{
		if (!u16str.empty())
		{
			auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &u16str[0], (int)u16str.size(), NULL, 0, NULL, NULL);
			std::string u8str(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, &u16str[0], (int)u16str.size(), &u8str[0], size_needed, NULL, NULL);
			return u8str;
		}
		return std::string();
	}
}
