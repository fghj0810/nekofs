#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <map>

namespace nekofs {
	class LayerFileSystem final : public FileSystem, private noncopyable, private nonmovable, public std::enable_shared_from_this<LayerFileSystem>
	{
	public:
		static std::shared_ptr<LayerFileSystem> createNativeLayer(const fsstring& dirpath);
		fsstring getFullPath(const fsstring& path) const;

	public:
		fsstring getCurrentPath() const override;
		std::vector<fsstring> getAllFiles(const fsstring& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const fsstring& filepath) override;
		std::shared_ptr<IStream> openIStream(const fsstring& filepath) override;
		bool fileExist(const fsstring& filepath) const override;
		bool dirExist(const fsstring& dirpath) const override;
		int64_t getSize(const fsstring& filepath) const override;
		FileSystemType getFSType() const override;

	private:
		std::shared_ptr<FileSystem> filesystem_;
		fsstring currentpath_;
	};
}
