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
		static std::shared_ptr<LayerFileSystem> createNativeLayer(const std::string& dirpath);
		std::string getFullPath(const std::string& path) const;
		std::string getFileURI(const std::string& path) const;

	public:
		std::string getCurrentPath() const override;
		std::vector<std::string> getAllFiles(const std::string& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) override;
		std::shared_ptr<IStream> openIStream(const std::string& filepath) override;
		FileType getFileType(const std::string& path) const override;
		int64_t getSize(const std::string& filepath) const override;
		FileSystemType getFSType() const override;

	private:
		std::shared_ptr<FileSystem> filesystem_;
		std::string currentpath_;
	};
}
