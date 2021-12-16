#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <map>

namespace nekofs {
	class NativeIStream;
	class NativeOStream;
	class NativeFile;

	class NativeFileSystem final : public FileSystem, private noncopyable, private nonmovable, public std::enable_shared_from_this<NativeFileSystem>
	{
	public:
		std::string getCurrentPath() const override;
		std::vector<std::string> getAllFiles(const std::string& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) override;
		std::shared_ptr<IStream> openIStream(const std::string& filepath) override;
		FileType getFileType(const std::string& path) const override;
		int64_t getSize(const std::string& filepath) const override;
		FileSystemType getFSType() const override;

	public:
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
}
