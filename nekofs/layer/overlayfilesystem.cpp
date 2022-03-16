#include "overlayfilesystem.h"
#include "../common/env.h"
#include "../common/utils.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif
#include "../nekodata/nekodatafilesystem.h"

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
			return  it->second.first->getFileHandle(it->second.second);
		}
		return nullptr;
	}
	std::shared_ptr<IStream> OverlayFileSystem::openIStream(const std::string& filepath)
	{
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			return  it->second.first->openIStream(it->second.second);
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
			return  it->second.first->getSize(it->second.second);
		}
		return -1;
	}
	FileSystemType OverlayFileSystem::getFSType() const
	{
		return FileSystemType::Overlay;
	}

	bool OverlayFileSystem::addLayer(std::shared_ptr<FileSystem> fs, const std::string& dirpath)
	{
		std::string parentDir = "";
		if (!dirpath.empty())
		{
			parentDir = dirpath + nekofs_PathSeparator;
		}
		auto version = LayerVersionMeta::load(fs->openIStream(parentDir + nekofs_kLayerVersion));
		auto files = LayerFilesMeta::load(fs->openIStream(parentDir + nekofs_kLayerFiles));
		if (!version)
		{
			std::stringstream ss;
			ss << u8"OverlayFileSystem::addLayer can not open ";
			ss << parentDir + nekofs_kLayerVersion;
			logerr(ss.str());
			return false;
		}
		if (!files)
		{
			std::stringstream ss;
			ss << u8"OverlayFileSystem::addLayer can not open ";
			ss << parentDir + nekofs_kLayerFiles;
			logerr(ss.str());
			return false;
		}
		if (!layers_.empty())
		{
			// 版本要连续
			const auto& lastver = std::get<1>(layers_.back());
			if (version->getFromVersion() > lastver.getVersion() || version->getVersion() <= lastver.getVersion())
			{
				std::stringstream ss;
				ss << u8"OverlayFileSystem::addLayer error due to (fromVersion > lastVer || version <= lastVer). lastVer = ";
				ss << lastver.getVersion();
				ss << u8", new fromVersion = ";
				ss << version->getFromVersion();
				ss << u8", new version = ";
				ss << version->getVersion();
				logerr(ss.str());
				return false;
			}
		}
		else
		{
			// 版本必须从0开始
			if (version->getFromVersion() != 0)
			{
				std::stringstream ss;
				ss << u8"OverlayFileSystem::addLayer error due to version->getFromVersion() != 0). ";
				ss << u8"fromVersion = ";
				ss << version->getFromVersion();
				logerr(ss.str());
				return false;
			}
		}
		layers_.push_back(std::make_tuple(fs, version.value(), files.value(), parentDir));
		return true;
	}
	bool OverlayFileSystem::refreshFileList()
	{
		lvm_.reset();
		lfm_.reset();
		files_.clear();
		LayerFilesMeta tmp_lfm;
		std::map<std::string, std::pair<std::shared_ptr<FileSystem>, std::string>> tmp_files;
		if (layers_.empty())
		{
			return false;
		}
		for (size_t i = 0; i < layers_.size(); i++)
		{
			auto& fs = std::get<0>(layers_[i]);
			const LayerFilesMeta& lfm = std::get<2>(layers_[i]);
			const std::string& parentDir = std::get<3>(layers_[i]);
			const auto& files = lfm.getFiles();
			for (const auto& f : files)
			{
				tmp_files[f.first] = std::make_pair(fs, parentDir + f.first);
				tmp_lfm.setFileMeta(f.first, lfm.getFileMeta(f.first).value());
			}
			const auto& deletes = lfm.getDeletes();
			for (const auto& f : deletes)
			{
				auto meta = tmp_lfm.getFileMeta(f.first);
				if (meta.has_value() && f.second > meta->getVersion())
				{
					// version相等不做删除，可能是在散文件和nekodata中挪动
					tmp_files.erase(f.first);
					tmp_lfm.purgeFile(f.first);
				}
			}
			const auto& nekodatas = lfm.getNekodatas();
			for (const auto& nekodata : nekodatas)
			{
				auto nekodatafs = NekodataFileSystem::create(fs, parentDir + nekodata);
				if (!nekodatafs)
				{
					return false;
				}
				auto prefixPath = nekodata.substr(0, nekodata.size() - nekofs_kNekodata_FileExtension.size()) + nekofs_PathSeparator;
				if (!refreshFileList(tmp_files, tmp_lfm, nekodatafs, prefixPath))
				{
					return false;
				}
			}
		}
		files_ = tmp_files;
		lvm_ = std::get<1>(layers_.back());
		lvm_->setFromVersion(std::get<1>(layers_.back()).getFromVersion());
		lfm_ = tmp_lfm;
		return true;
	}
	bool OverlayFileSystem::refreshFileList(FileMap& tmp_files, LayerFilesMeta& tmp_lfm, std::shared_ptr<NekodataFileSystem> fs, const std::string& prefixPath)
	{
		auto lfm = LayerFilesMeta::load(fs->openIStream(nekofs_kLayerFiles));
		if (!lfm.has_value())
		{
			std::stringstream ss;
			ss << u8"OverlayFileSystem::refreshFileList can not open ";
			ss << prefixPath + nekofs_kLayerFiles;
			logerr(ss.str());
			return false;
		}
		const auto& files = lfm->getFiles();
		for (const auto& f : files)
		{
			tmp_files[prefixPath + f.first] = std::make_pair(fs, f.first);
			tmp_lfm.setFileMeta(prefixPath + f.first, lfm->getFileMeta(f.first).value());
		}
		const auto& deletes = lfm->getDeletes();
		for (const auto& f : deletes)
		{
			auto meta = tmp_lfm.getFileMeta(prefixPath + f.first);
			if (meta.has_value() && f.second > meta->getVersion())
			{
				// version相等不做删除，可能是在散文件和nekodata中挪动
				tmp_files.erase(prefixPath + f.first);
				tmp_lfm.purgeFile(prefixPath + f.first);
			}
		}
		const auto& nekodatas = lfm->getNekodatas();
		for (const auto& nekodata : nekodatas)
		{
			auto nekodatafs = NekodataFileSystem::create(fs, nekodata);
			if (!nekodatafs)
			{
				return false;
			}
			auto tmp_prefixPath = nekodata.substr(0, nekodata.size() - nekofs_kNekodata_FileExtension.size()) + nekofs_PathSeparator;
			if (!refreshFileList(tmp_files, tmp_lfm, nekodatafs, prefixPath + tmp_prefixPath))
			{
				return false;
			}
		}
		return true;
	}
	std::optional<LayerVersionMeta> OverlayFileSystem::getVersion() const
	{
		return lvm_;
	}
	std::optional<LayerFilesMeta> OverlayFileSystem::getFiles() const
	{
		return lfm_;
	}
	std::string OverlayFileSystem::getFileURI(const std::string& filepath) const
	{
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			switch (it->second.first->getFSType())
			{
			case FileSystemType::Native:
				return nekofs_kURIPrefix_Native + it->second.second;
			case FileSystemType::Nekodata:
				return nekofs_kURIPrefix_Nekodata + it->second.second;
			default:
				break;
			}
		}
		return std::string();
	}
}
