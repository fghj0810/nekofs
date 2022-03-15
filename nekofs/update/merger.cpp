#include "merger.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#include "../layer/layerfilesystem.h"
#include "../layer/layerfilesmeta.h"
#include "../layer/layerversionmeta.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif
#include "../nekodata/nekodatafilesystem.h"
#include "../nekodata/nekodataarchiver.h"

#include <sstream>
#include <functional>

namespace nekofs {
	Merger::Merger(const std::string& resName, uint32_t baseVersion)
	{
		resName_ = resName;
		baseVersion_ = baseVersion;
	}
	bool Merger::addPatch(std::shared_ptr<FileSystem> fs)
	{
		auto lvm = LayerVersionMeta::load(fs->openIStream(nekofs_kLayerVersion));
		if (lvm.has_value())
		{
			if (!patchfs_.empty())
			{
				// 判断版本是否和已加入的连续
				const auto& last = std::get<1>(patchfs_.back());
				if (last.getFromVersion() <= lvm->getFromVersion() && last.getVersion() < lvm->getVersion())
				{
					patchfs_.push_back(std::make_tuple(fs, lvm.value()));
					return true;
				}
				else
				{
					std::stringstream ss;
					ss << u8"Merger::addPatch version err.";
					ss << u8" last.from = " << last.getFromVersion();
					ss << u8", last.to = " << last.getVersion();
					ss << u8", patch.from = " << lvm->getFromVersion();
					ss << u8", patch.to = " << lvm->getVersion();
					logerr(ss.str());
					return false;
				}
			}
			else
			{
				if (lvm->getFromVersion() <= baseVersion_)
				{
					patchfs_.push_back(std::make_tuple(fs, lvm.value()));
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		return false;
	}
	bool Merger::exec(const std::string& outdir, int64_t volumeSize)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		if (nativefs->getFileType(outdir) != FileType::None)
		{
			// 目录已存在
			return false;
		}
		if (!nativefs->createDirectories(outdir))
		{
			// 无法创建目录
			return false;
		}
		std::vector<std::shared_ptr<FileSystem>> fslist;
		for (auto& item : patchfs_)
		{
			fslist.push_back(std::get<0>(item));
		}
		{
			// 计算要合并的文件列表
			int64_t total = 0;
			if (!prepare(total, fslist))
			{
				return false;
			}
			std::lock_guard lock(mtx_);
			complete_ = 0;
			total_ = total + 1; // 额外加上 nekofs_kLayerVersion
		}
		LayerVersionMeta lvm = std::get<1>(patchfs_.back());
		lvm.setFromVersion(baseVersion_);
		auto lfm = getLayerFilesMeta(fslist);
		const auto& allfiles = lfm->getFiles();
		for (const auto& file : allfiles)
		{
			if (!copyfile(getIStream(fslist, file.first), nativefs->openOStream(outdir + nekofs_PathSeparator + file.first)))
			{
				return false;
			}
			addComplete();
		}
		const auto& allnekodatas = lfm->getNekodatas();
		for (const auto& nekodata : allnekodatas)
		{
			auto archiver = std::make_shared<NekodataArchiver>(outdir + nekofs_PathSeparator + nekodata, volumeSize);
			std::vector<std::shared_ptr<FileSystem>> fslist_nekodata;
			for (auto fs : fslist)
			{
				auto nekodatafs = NekodataFileSystem::create(fs, nekodata);
				if (nekodatafs)
				{
					fslist_nekodata.push_back(nekodatafs);
				}
			}
			exec(archiver, fslist_nekodata);
			if (!archiver->archive(std::bind(&Merger::addComplete, this)))
			{
				return false;
			}
			addComplete();
		}
		if (!lvm.save(nativefs->openOStream(outdir + nekofs_PathSeparator + nekofs_kLayerVersion)))
		{
			return false;
		}
		addComplete();
		if (!lfm->save(nativefs->openOStream(outdir + nekofs_PathSeparator + nekofs_kLayerFiles)))
		{
			return false;
		}
		addComplete();
		return true;
	}
	bool Merger::exec(std::shared_ptr<NekodataArchiver> archiver)
	{
		std::vector<std::shared_ptr<FileSystem>> fslist;
		for (auto& item : patchfs_)
		{
			fslist.push_back(std::get<0>(item));
		}
		{
			// 计算要合并的文件列表
			int64_t total = 0;
			if (!prepare(total, fslist))
			{
				return false;
			}
			std::lock_guard lock(mtx_);
			complete_ = 0;
			total_ = total + 1; // 额外加上 nekofs_kLayerVersion
		}
		LayerVersionMeta lvm = std::get<1>(patchfs_.back());
		lvm.setFromVersion(baseVersion_);
		auto jsonStrBuffer_lvm = newJsonBuffer();
		JSONStringPrettyWriter jsonString_lfm(*jsonStrBuffer_lvm);
		JSONDocument d_lvm(rapidjson::kObjectType);
		lvm.save(&d_lvm, d_lvm.GetAllocator());
		d_lvm.Accept(jsonString_lfm);
		archiver->addBuffer(nekofs_kLayerVersion, jsonStrBuffer_lvm->GetString(), static_cast<int64_t>(jsonStrBuffer_lvm->GetSize()));
		exec(archiver, fslist);
		return archiver->archive(std::bind(&Merger::addComplete, this));
	}
	bool Merger::getProgress(int64_t& complete, int64_t& total)
	{
		std::lock_guard lock(mtx_);
		complete = complete_;
		total = total_;
		return total > 0;
	}
	bool Merger::prepare(int64_t& total, const std::vector<std::shared_ptr<FileSystem>>& fslist)
	{
		auto lfm = getLayerFilesMeta(fslist);
		if (!lfm.has_value())
		{
			return false;
		}
		// 加上所有散文件
		total += lfm->getFiles().size();
		total++; // 加上files.json自身
		const auto& nekodatas = lfm->getNekodatas();
		for (auto nekodata : nekodatas)
		{
			std::vector<std::shared_ptr<FileSystem>> fslist_nekodata;
			for (auto fs : fslist)
			{
				auto nekodatafs = NekodataFileSystem::create(fs, nekodata);
				if (nekodatafs)
				{
					fslist_nekodata.push_back(nekodatafs);
				}
			}
			if (!fslist_nekodata.empty())
			{
				if (!prepare(total, fslist_nekodata))
				{
					return false;
				}
				// 加上自己.nekodata
				total++;
			}
		}
		return true;
	}
	void Merger::exec(std::shared_ptr<NekodataArchiver> archiver, const std::vector<std::shared_ptr<FileSystem>>& fslist)
	{
		auto lfm = getLayerFilesMeta(fslist);
		const auto& allfiles = lfm->getFiles();
		for (const auto& file : allfiles)
		{
			auto streamInfo = tryGetIStream(fslist, file.first);
			if (streamInfo.rawis != nullptr)
			{
				archiver->addRawFile(file.first, streamInfo.rawis, streamInfo.meta);
			}
			else
			{
				archiver->addFile(file.first, streamInfo.fs, file.first);
			}
		}
		const auto& allnekodatas = lfm->getNekodatas();
		for (const auto& nekodata : allnekodatas)
		{
			auto archiver_tmp = archiver->addArchive(nekodata);
			auto func = std::bind(&Merger::addComplete, this);
			std::vector<std::shared_ptr<FileSystem>> fslist_nekodata;
			for (auto fs : fslist)
			{
				auto nekodatafs = NekodataFileSystem::create(fs, nekodata);
				if (nekodatafs)
				{
					fslist_nekodata.push_back(nekodatafs);
				}
			}
			exec(archiver_tmp, fslist_nekodata);
		}
		auto jsonStrBuffer_lfm = newJsonBuffer();
		JSONStringPrettyWriter jsonString_lfm(*jsonStrBuffer_lfm);
		JSONDocument d_lfm(rapidjson::kObjectType);
		lfm->save(&d_lfm, d_lfm.GetAllocator());
		d_lfm.Accept(jsonString_lfm);
		archiver->addBuffer(nekofs_kLayerFiles, jsonStrBuffer_lfm->GetString(), static_cast<int64_t>(jsonStrBuffer_lfm->GetSize()));
	}
	std::optional<LayerFilesMeta> Merger::getLayerFilesMeta(const std::vector<std::shared_ptr<FileSystem>>& fslist)
	{
		std::vector<LayerFilesMeta> lfms;
		for (auto fs : fslist)
		{
			auto lfm_fs = LayerFilesMeta::load(fs->openIStream(nekofs_kLayerFiles));
			if (lfm_fs.has_value())
			{
				lfms.push_back(lfm_fs.value());
			}
			else
			{
				return std::nullopt;
			}
		}
		return LayerFilesMeta::merge(lfms, baseVersion_);
	}
	std::shared_ptr<IStream> Merger::getIStream(const std::vector<std::shared_ptr<FileSystem>>& fslist, const std::string& filepath)
	{
		for (auto fs : fslist)
		{
			auto is = fs->openIStream(filepath);
			if (is)
			{
				return is;
			}
		}
		return nullptr;
	}
	Merger::RawStreamInfo Merger::tryGetIStream(const std::vector<std::shared_ptr<FileSystem>>& fslist, const std::string& filepath)
	{
		Merger::RawStreamInfo info;
		for (auto fs : fslist)
		{
			auto ftype = fs->getFileType(filepath);
			if (ftype == FileType::Regular)
			{
				info.fs = fs;
				if (fs->getFSType() == FileSystemType::Nekodata)
				{
					std::shared_ptr<NekodataFileSystem> nekodatafs = std::static_pointer_cast<NekodataFileSystem>(fs);
					auto is = nekodatafs->openRawIStream(filepath);
					if (is)
					{
						info.rawis = is;
						info.meta = nekodatafs->getFileMeta(filepath).value();
					}
				}
				break;
			}
		}
		return info;
	}
	void Merger::addComplete()
	{
		std::lock_guard lock(mtx_);
		complete_++;
	}
	std::shared_ptr<JSONStringBuffer> Merger::newJsonBuffer()
	{
		auto buffer = std::make_shared<JSONStringBuffer>();
		jsonBuffer_.push_back(buffer);
		return buffer;
	}
}
