#include "overlayfilesystem.h"
#include "layerfilesystem.h"
#include "../common/env.h"
#include "../common/utils.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

#include <sstream>

namespace nekofs {
	std::string OverlayFileSystem::getCurrentPath() const
	{
		return std::string();
	}
	std::vector<std::string> OverlayFileSystem::getAllFiles(const std::string& dirpath) const
	{
		std::vector<std::string> allfiles(files_.size());
		size_t i = 0;
		for (const auto& item : files_)
		{
			allfiles[i] = item.first;
			i++;
		}
		return allfiles;
	}
	std::unique_ptr<FileHandle> OverlayFileSystem::getFileHandle(const std::string& filepath)
	{
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			return  std::get<0>(layers_[it->second.first])->getFileHandle(it->second.second);
		}
		return nullptr;
	}
	std::shared_ptr<IStream> OverlayFileSystem::openIStream(const std::string& filepath)
	{
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			return  std::get<0>(layers_[it->second.first])->openIStream(it->second.second);
		}
		return nullptr;
	}
	FileType OverlayFileSystem::getFileType(const std::string& path) const
	{
		auto it = files_.find(path);
		if (it != files_.end())
		{
			return FileType::Regular;
		}
		return FileType::None;
	}
	int64_t OverlayFileSystem::getSize(const std::string& filepath) const
	{
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			return  std::get<0>(layers_[it->second.first])->getSize(it->second.second);
		}
		return -1;
	}
	FileSystemType OverlayFileSystem::getFSType() const
	{
		return FileSystemType::Overlay;
	}

	bool OverlayFileSystem::addNativeLayer(const std::string& dirpath)
	{
		auto fs = LayerFileSystem::createNativeLayer(dirpath);
		auto version = LayerVersionMeta::load(fs->openIStream(nekofs_kLayerVersion));
		auto files = LayerFilesMeta::load(fs->openIStream(nekofs_kLayerFiles));
		if (!version)
		{
			std::stringstream ss;
			ss << u8"OverlayFileSystem::addNativeLayer can not open ";
			ss << fs->getFullPath(nekofs_kLayerVersion);
			logerr(ss.str());
			return false;
		}
		if (!files)
		{
			std::stringstream ss;
			ss << u8"OverlayFileSystem::addNativeLayer can not open ";
			ss << fs->getFullPath(nekofs_kLayerFiles);
			logerr(ss.str());
			return false;
		}
		if (!layers_.empty())
		{
			const auto& lastver = std::get<1>(layers_.back());
			if (version->getFromVersion() > lastver.getVersion() || version->getVersion() <= lastver.getVersion())
			{
				std::stringstream ss;
				ss << u8"OverlayFileSystem::addNativeLayer error due to (fromVersion > lastVer || version <= lastVer). lastVer = ";
				ss << lastver.getVersion();
				ss << u8", new fromVersion = ";
				ss << version->getFromVersion();
				ss << u8", new version = ";
				ss << version->getVersion();
				logerr(ss.str());
				return false;
			}
		}
		layers_.push_back(std::make_tuple(fs, version.value(), files.value()));
		return true;
	}
	void OverlayFileSystem::refreshFileList()
	{
		files_.clear();
		if (layers_.empty())
		{
			return;
		}
		std::vector<LayerFilesMeta> lfms(layers_.size());
		for (size_t i = 0; i < layers_.size(); i++)
		{
			const LayerFilesMeta& lfm = std::get<2>(layers_[i]);
			for (const auto& f : lfm.getFiles())
			{
				files_[f.first] = std::make_pair(i, f.first);
			}
			for (const auto& f : lfm.getDeletes())
			{
				files_.erase(f.first);
			}
		}

	}
	std::optional<LayerVersionMeta> OverlayFileSystem::getVersion() const
	{
		if (layers_.empty())
		{
			return std::nullopt;
		}
		LayerVersionMeta lvm;
		lvm = std::get<1>(layers_.back());
		lvm.setFromVersion(std::get<1>(layers_.front()).getFromVersion());
		return lvm;
	}
	std::string OverlayFileSystem::getFileURI(const std::string& filepath) const
	{
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			return std::get<0>(layers_[it->second.first])->getFileURI(it->second.second);
		}
		return std::string();
	}
}
