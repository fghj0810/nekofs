#pragma once

#include "../common/typedef.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <map>

namespace nekofs {
	class AssetManagerFile;

	class AssetManagerFileSystem final : public FileSystem, public std::enable_shared_from_this<AssetManagerFileSystem>
	{
	private:
		AssetManagerFileSystem(const AssetManagerFileSystem&) = delete;
		AssetManagerFileSystem(AssetManagerFileSystem&&) = delete;
		AssetManagerFileSystem& operator=(const AssetManagerFileSystem&) = delete;
		AssetManagerFileSystem& operator=(AssetManagerFileSystem&&) = delete;

	public:
		std::string getCurrentPath() const override;
		std::vector<std::string> getAllFiles(const std::string& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) override;
		std::shared_ptr<IStream> openIStream(const std::string& filepath) override;
		FileType getFileType(const std::string& path) const override;
		int64_t getSize(const std::string& filepath) const override;
		FileSystemType getFSType() const override;

	public:
		AssetManagerFileSystem() = default;
		std::vector<std::string> getFiles(const std::string& dirpath) const;

	private:
		static void weakDeleteCallback(std::weak_ptr<AssetManagerFileSystem> filesystem, AssetManagerFile* file);
		std::shared_ptr<AssetManagerFile> openFileInternal(const std::string& filepath);
		void closeFileInternal(const std::string& filepath);

	private:
		std::map<std::string, std::weak_ptr<AssetManagerFile>> files_;
		std::map<std::string, AssetManagerFile*> filePtrs_;
		std::recursive_mutex mtx_;
	};
}
