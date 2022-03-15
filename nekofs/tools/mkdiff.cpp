#include "mkdiff.h"
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

namespace nekofs::tools {
	bool MKDiff::exec(const std::string& earlierfile, const std::string& latestfile, const std::string& filepath, int64_t volumeSize)
	{
		auto nativefs = env::getInstance().getNativeFileSystem();
		// 检查文件是否已存在，如果存在就报错退出
		if (auto ft = nativefs->getFileType(filepath); ft != nekofs::FileType::None)
		{
			std::stringstream ss;
			ss << u8"filepath already exist! ";
			ss << filepath;
			nekofs::logerr(ss.str());
			return false;
		}
		// 检查较早的文件是否存在
		if (auto ft = nativefs->getFileType(earlierfile); ft != nekofs::FileType::Regular)
		{
			std::stringstream ss;
			ss << u8"earlierfile not found! ";
			ss << earlierfile;
			nekofs::logerr(ss.str());
			return false;
		}
		// 检查最新的文件是否存在
		if (auto ft = nativefs->getFileType(latestfile); ft != nekofs::FileType::Regular)
		{
			std::stringstream ss;
			ss << u8"latestfile not found! ";
			ss << latestfile;
			nekofs::logerr(ss.str());
			return false;
		}
		auto earlierfs = NekodataFileSystem::createFromNative(earlierfile);
		auto latestfs = NekodataFileSystem::createFromNative(latestfile);
		nekofs::loginfo(u8"verify " + earlierfile + u8" ...");
		if (!earlierfs || !earlierfs->verify())
		{
			nekofs::logerr(u8"verify " + earlierfile + u8" ... failed");
			return false;
		}
		nekofs::loginfo(u8"verify " + earlierfile + u8" ... ok");
		nekofs::loginfo(u8"verify " + latestfile + u8" ...");
		if (!latestfs || !latestfs->verify())
		{
			nekofs::logerr(u8"verify " + latestfile + u8" ... failed");
			return false;
		}
		nekofs::loginfo(u8"verify " + latestfile + u8" ... ok");

		auto vm_earlier = nekofs::LayerVersionMeta::load(earlierfs->openIStream(nekofs_kLayerVersion));
		auto vm_latest = nekofs::LayerVersionMeta::load(latestfs->openIStream(nekofs_kLayerVersion));
		if (!vm_earlier.has_value())
		{
			nekofs::logerr(u8"earlierfile: can not open version file.");
			return false;
		}
		if (!vm_latest.has_value())
		{
			nekofs::logerr(u8"latestfile: can not open version file.");
			return false;
		}
		if (vm_latest->getVersion() <= vm_earlier->getVersion())
		{
			nekofs::logerr(u8"vm_latest->getVersion() <= vm_earlier->getVersion()");
			return false;
		}

		auto fm_earlier = nekofs::LayerFilesMeta::load(earlierfs->openIStream(nekofs_kLayerFiles));
		auto fm_latest = nekofs::LayerFilesMeta::load(latestfs->openIStream(nekofs_kLayerFiles));
		if (!fm_earlier.has_value())
		{
			nekofs::logerr(u8"earlierfile: can not open filesmeta.");
			return false;
		}
		if (!fm_latest.has_value())
		{
			nekofs::logerr(u8"latestfile: can not open filesmeta.");
			return false;
		}

		auto lvm = vm_latest.value();
		lvm.setFromVersion(vm_earlier->getVersion());
		// versionmeta和filesmeta都没有问题。准备对比差异。
		auto lfm = nekofs::LayerFilesMeta::makediff(fm_earlier.value(), fm_latest.value(), vm_latest->getVersion());

		auto jsonStrBuffer_lvm = newJsonBuffer();
		JSONStringPrettyWriter jsonString_lvm(*jsonStrBuffer_lvm);
		JSONDocument d_lvm(rapidjson::kObjectType);
		lvm.save(&d_lvm, d_lvm.GetAllocator());
		d_lvm.Accept(jsonString_lvm);
		auto jsonStrBuffer_lfm = newJsonBuffer();
		JSONStringPrettyWriter jsonString_lfm(*jsonStrBuffer_lfm);
		JSONDocument d_lfm(rapidjson::kObjectType);
		lfm.save(&d_lfm, d_lfm.GetAllocator());
		d_lfm.Accept(jsonString_lfm);

		auto archiver = std::make_shared<NekodataArchiver>(filepath, volumeSize);
		archiver->addBuffer(nekofs_kLayerVersion, jsonStrBuffer_lvm->GetString(), static_cast<int64_t>(jsonStrBuffer_lvm->GetSize()));
		archiver->addBuffer(nekofs_kLayerFiles, jsonStrBuffer_lfm->GetString(), static_cast<int64_t>(jsonStrBuffer_lfm->GetSize()));
		const auto& files = lfm.getFiles();
		for (const auto& item : files)
		{
			auto meta = latestfs->getFileMeta(item.first);
			auto is = latestfs->openRawIStream(item.first);
			if (meta.has_value() && is)
			{
				archiver->addRawFile(item.first, is, meta.value());
			}
			else
			{
				nekofs::logerr(u8"!get nekodata.filemeta failed! file = " + item.first);
				return false;
			}
		}
		const auto& nekodatas = lfm.getNekodatas();
		for (const auto& item : nekodatas)
		{
			auto subArchiver = archiver->addArchive(item);
			auto earliersubfs = NekodataFileSystem::create(earlierfs, item);
			auto latestsubfs = NekodataFileSystem::create(latestfs, item);
			if (!diffLayer(subArchiver, earliersubfs, latestsubfs, vm_latest->getVersion()))
			{
				nekofs::logerr(u8"!diffLayer(subArchiver, earliersubfs, latestsubfs)");
				return false;
			}
		}

		return archiver->archive();
	}
	bool MKDiff::diffLayer(std::shared_ptr<NekodataArchiver> archiver, std::shared_ptr<NekodataFileSystem> earlierfs, std::shared_ptr<NekodataFileSystem> latestfs, uint32_t latestVersion)
	{
		auto fm_earlier = earlierfs ? nekofs::LayerFilesMeta::load(earlierfs->openIStream(nekofs_kLayerFiles)) : std::nullopt;
		auto fm_latest = latestfs ? nekofs::LayerFilesMeta::load(latestfs->openIStream(nekofs_kLayerFiles)) : std::nullopt;
		if (!fm_earlier.has_value())
		{
			fm_earlier = nekofs::LayerFilesMeta();
		}
		if (!fm_latest.has_value())
		{
			fm_latest = nekofs::LayerFilesMeta();
		}
		auto lfm = nekofs::LayerFilesMeta::makediff(fm_earlier.value(), fm_latest.value(), latestVersion);
		auto jsonStrBuffer_lfm = newJsonBuffer();
		JSONStringPrettyWriter jsonString_lfm(*jsonStrBuffer_lfm);
		JSONDocument d_lfm(rapidjson::kObjectType);
		lfm.save(&d_lfm, d_lfm.GetAllocator());
		d_lfm.Accept(jsonString_lfm);

		archiver->addBuffer(nekofs_kLayerFiles, jsonStrBuffer_lfm->GetString(), static_cast<int64_t>(jsonStrBuffer_lfm->GetSize()));

		const auto& files = lfm.getFiles();
		for (const auto& item : files)
		{
			auto meta = latestfs->getFileMeta(item.first);
			auto is = latestfs->openRawIStream(item.first);
			if (meta.has_value() && is)
			{
				archiver->addRawFile(item.first, is, meta.value());
			}
			else
			{
				nekofs::logerr(u8"diffLayer: get nekodata.filemeta failed! file = " + item.first);
				return false;
			}
		}
		const auto& nekodatas = lfm.getNekodatas();
		for (const auto& item : nekodatas)
		{
			auto subArchiver = archiver->addArchive(item);
			auto earliersubfs = NekodataFileSystem::create(earlierfs, item);
			auto latestsubfs = NekodataFileSystem::create(latestfs, item);
			if (!diffLayer(subArchiver, earliersubfs, latestsubfs, latestVersion))
			{
				nekofs::logerr(u8"diffLayer: !diffLayer(subArchiver, earliersubfs, latestsubfs)");
				return false;
			}
		}
		return true;
	}
	std::shared_ptr<JSONStringBuffer> MKDiff::newJsonBuffer()
	{
		auto buffer = std::make_shared<JSONStringBuffer>();
		jsonBuffer_.push_back(buffer);
		return buffer;
	}
}
