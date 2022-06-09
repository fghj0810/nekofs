#pragma once

#include "../common/typedef.h"
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
	class NekodataFileSystem;

	class OverlayFileSystem final : public FileSystem, public std::enable_shared_from_this<OverlayFileSystem>
	{
		typedef std::map<std::string, std::pair<std::shared_ptr<FileSystem>, std::string>> FileMap;

	private:
		OverlayFileSystem(const OverlayFileSystem&) = delete;
		OverlayFileSystem(const OverlayFileSystem&&) = delete;
		OverlayFileSystem& operator=(const OverlayFileSystem&) = delete;
		OverlayFileSystem& operator=(const OverlayFileSystem&&) = delete;

	public:
		std::string getCurrentPath() const override;
		std::vector<std::string> getAllFiles(const std::string& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) override;
		std::shared_ptr<IStream> openIStream(const std::string& filepath) override;
		FileType getFileType(const std::string& path) const override;
		int64_t getSize(const std::string& filepath) const override;
		FileSystemType getFSType() const override;

	public:
		OverlayFileSystem() = default;
		bool addLayer(std::shared_ptr<FileSystem> fs, const std::string& dirpath = "");
		bool refreshFileList();
		bool refreshFileList(FileMap& tmp_files, LayerFilesMeta& tmp_lfm, std::shared_ptr<NekodataFileSystem> fs, const std::string& prefixPath);
		std::optional<LayerVersionMeta> getVersion() const;
		std::optional<LayerFilesMeta> getFiles() const;
		std::string getFileURI(const std::string& filepath) const;

	private:
		std::vector<std::tuple<std::shared_ptr<FileSystem>, LayerVersionMeta, LayerFilesMeta, std::string>> layers_;
		FileMap files_;
		std::optional<LayerVersionMeta> lvm_;
		std::optional<LayerFilesMeta> lfm_;
	};
}
