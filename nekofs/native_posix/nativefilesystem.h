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
		fsstring getCurrentPath() const override;
		std::vector<fsstring> getAllFiles(const fsstring& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const fsstring& filepath) override;
		std::shared_ptr<IStream> openIStream(const fsstring& filepath) override;
		bool fileExist(const fsstring& filepath) const override;
		bool dirExist(const fsstring& dirpath) const override;
		int64_t getSize(const fsstring& filepath) const override;
		FileSystemType getFSType() const override;

	public:
		bool createDirectories(const fsstring& dirpath);
		bool removeDirectories(const fsstring& dirpath);
		bool cleanEmptyDirectories(const fsstring& dirpath);
		bool moveDirectory(const fsstring& srcpath, const fsstring& destpath);
		bool removeFile(const fsstring& filepath);
		bool moveFile(const fsstring& srcpath, const fsstring& destpath);
		std::shared_ptr<OStream> openOStream(const fsstring& filepath);

	private:
		static void weakDeleteCallback(std::weak_ptr<NativeFileSystem> filesystem, NativeFile* file);
		std::shared_ptr<NativeFile> openFileInternal(const fsstring& filepath);
		void closeFileInternal(const fsstring& filepath);
		bool hasOpenFiles(const fsstring& dirpath) const;
		ItemType getItemType(const fsstring& path) const;

	private:
		std::map<fsstring, std::weak_ptr<NativeFile>> files_;
		std::map<fsstring, NativeFile*> filePtrs_;
		std::recursive_mutex mtx_;
	};
}
