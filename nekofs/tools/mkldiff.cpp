﻿#include "mkldiff.h"
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
	bool MKLDiff::exec(const std::string& filepath, const std::string& earlierfile, const std::string& latestfile)
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
		nekofs::loginfo(u8"verify earlierfile ...");
		if (!earlierfs || !earlierfs->verify())
		{
			nekofs::logerr(u8"verify earlierfile ... failed");
			return false;
		}
		nekofs::loginfo(u8"verify latestfile ...");
		if (!latestfs || !latestfs->verify())
		{
			nekofs::logerr(u8"verify latestfile ... failed");
			return false;
		}
		nekofs::loginfo(u8"verify nekodata ... ok");

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

		nekofs::JSONStringBuffer jsonStrBuffer_lvm;
		JSONStringWriter jsonString_lvm(jsonStrBuffer_lvm);
		JSONDocument d_lvm(rapidjson::kObjectType);
		lvm.save(&d_lvm, d_lvm.GetAllocator());
		d_lvm.Accept(jsonString_lvm);
		nekofs::JSONStringBuffer jsonStrBuffer_lfm;
		JSONStringWriter jsonString_lfm(jsonStrBuffer_lfm);
		JSONDocument d_lfm(rapidjson::kObjectType);
		lfm.save(&d_lfm, d_lfm.GetAllocator());
		d_lfm.Accept(jsonString_lfm);

		auto archiver = std::make_shared<NekodataNativeArchiver>(filepath);
		archiver->addFile(nekofs_kLayerVersion, jsonStrBuffer_lvm.GetString(), (uint32_t)jsonStrBuffer_lvm.GetSize());
		archiver->addFile(nekofs_kLayerFiles, jsonStrBuffer_lfm.GetString(), (uint32_t)jsonStrBuffer_lfm.GetSize());
		const auto& files = lfm.getFiles();
		for (const auto& item : files)
		{
			archiver->addFile(item.first, latestfs, item.first);
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
	bool MKLDiff::diffLayer(std::shared_ptr<NekodataNativeArchiver> archiver, std::shared_ptr<NekodataFileSystem> earlierfs, std::shared_ptr<NekodataFileSystem> latestfs, uint32_t latestVersion)
	{
		auto fm_earlier = nekofs::LayerFilesMeta::load(earlierfs->openIStream(nekofs_kLayerFiles));
		auto fm_latest = nekofs::LayerFilesMeta::load(latestfs->openIStream(nekofs_kLayerFiles));
		if (!fm_earlier.has_value())
		{
			fm_earlier = nekofs::LayerFilesMeta();
		}
		if (!fm_latest.has_value())
		{
			fm_latest = nekofs::LayerFilesMeta();
		}
		auto lfm = nekofs::LayerFilesMeta::makediff(fm_earlier.value(), fm_latest.value(), latestVersion);
		nekofs::JSONStringBuffer jsonStrBuffer_lfm;
		JSONStringWriter jsonString_lfm(jsonStrBuffer_lfm);
		JSONDocument d_lfm(rapidjson::kObjectType);
		lfm.save(&d_lfm, d_lfm.GetAllocator());
		d_lfm.Accept(jsonString_lfm);

		archiver->addFile(nekofs_kLayerVersion, jsonStrBuffer_lfm.GetString(), (uint32_t)jsonStrBuffer_lfm.GetSize());

		const auto& files = lfm.getFiles();
		for (const auto& item : files)
		{
			archiver->addFile(item.first, latestfs, item.first);
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
}