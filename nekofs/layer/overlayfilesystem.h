#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"
#include "layerversionmeta.h"
#include "layerfilesmeta.h"

#include <cstdint>
#include <memory>
#include <map>
#include <tuple>
#include <optional>

namespace nekofs {
	class LayerFileSystem;
	class LayerVersionMeta;
	class LayerFilesMeta;

	class OverlayFileSystem final : public FileSystem, private noncopyable, private nonmovable, public std::enable_shared_from_this<OverlayFileSystem>
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
		bool addNativeLayer(const std::string& dirpath);
		void refreshFileList();
		std::optional<LayerVersionMeta> getVersion() const;
		std::string getFileURI(const std::string& filepath) const;

	private:
		std::vector<std::tuple<std::shared_ptr<LayerFileSystem>, LayerVersionMeta, LayerFilesMeta>> layers_;
		std::map<std::string, std::pair<size_t, std::string>> files_;
	};
}
