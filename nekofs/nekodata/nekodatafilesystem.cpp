#include "nekodatafilesystem.h"
#include "nekodataistream.h"
#include "nekodatafile.h"
#include "util.h"
#include "../common/env.h"
#include "../common/sha256.h"
#include "../common/utils.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

#include <sstream>
#include <functional>

namespace nekofs {
	NekodataFileSystem::NekodataFileSystem(std::vector<std::shared_ptr<IStream>> v_is, int64_t volumeSize)
	{
		v_is_ = v_is;
		volumeSize_ = volumeSize;
	}
	std::string NekodataFileSystem::getCurrentPath() const
	{
		return std::string();
	}
	std::vector<std::string> NekodataFileSystem::getAllFiles(const std::string& dirpath) const
	{
		std::vector<std::string> allfiles;
		for (const auto& item : rawFiles_)
		{
			allfiles.push_back(item.first);
		}
		return allfiles;
	}
	std::unique_ptr<FileHandle> NekodataFileSystem::getFileHandle(const std::string& filepath)
	{
		return std::unique_ptr<FileHandle>();
	}
	std::shared_ptr<IStream> NekodataFileSystem::openIStream(const std::string& filepath)
	{
		auto file = openFileInternal(filepath);
		if (file)
		{
			return file->openIStream();
		}
		return nullptr;
	}
	FileType NekodataFileSystem::getFileType(const std::string& path) const
	{
		auto it = rawFiles_.find(path);
		if (it != rawFiles_.end())
		{
			return FileType::Regular;
		}
		return FileType::None;
	}
	int64_t NekodataFileSystem::getSize(const std::string& filepath) const
	{
		auto it = rawFiles_.find(filepath);
		if (it != rawFiles_.end())
		{
			return it->second.second.getOriginalSize();
		}
		return -1;
	}
	FileSystemType NekodataFileSystem::getFSType() const
	{
		return FileSystemType::Nekodata;
	}
	std::shared_ptr<NekodataFileSystem> NekodataFileSystem::createFromNative(const std::string& filepath)
	{
		if (filepath.size() < nekofs_kNekodata_FileExtension.size() + 1)
		{
			return nullptr;
		}
		auto nativefs = env::getInstance().getNativeFileSystem();
		auto is = nativefs->openIStream(filepath);
		if (!is || is->getLength() <= 8 + nekofs_kNekodata_VolumeFormatSize || is->seek(-nekofs_kNekodata_FileFooterSize, SeekOrigin::End) != is->getLength() - nekofs_kNekodata_FileFooterSize)
		{
			return nullptr;
		}
		bool success = true;
		uint32_t volume;
		uint32_t totalVolumes;
		int64_t volumeSize;
		success = success && nekodata_readVolumeNum(is, volume);
		success = success && nekodata_readVolumeNum(is, totalVolumes);
		success = success && nekodata_readVolumeSize(is, volumeSize);
		if (!success || totalVolumes == 0 || volume != 1 || (totalVolumes == 1 && is->getLength() > volumeSize) || (totalVolumes != 1 && is->getLength() != volumeSize))
		{
			return nullptr;
		}
		std::vector<std::shared_ptr<IStream>> v_is(totalVolumes);
		v_is[0] = is->createNew();
		if (totalVolumes > 1)
		{
			std::stringstream ss;
			uint32_t volume_tmp;
			uint32_t totalVolumes_tmp;
			int64_t volumeSize_tmp;
			for (uint32_t i = 1; i < totalVolumes - 1; i++)
			{
				ss.str(std::string());
				ss << filepath.substr(0, filepath.size() - nekofs_kNekodata_FileExtension.size());
				ss << u8"." << i;
				ss << nekofs_kNekodata_FileExtension;
				auto is_tmp = nativefs->openIStream(ss.str());
				if (!is_tmp || is_tmp->getLength() != volumeSize || is_tmp->seek(-nekofs_kNekodata_FileFooterSize, SeekOrigin::End) != is_tmp->getLength() - nekofs_kNekodata_FileFooterSize)
				{
					return nullptr;
				}
				success = success && nekodata_readVolumeNum(is_tmp, volume_tmp);
				success = success && nekodata_readVolumeNum(is_tmp, totalVolumes_tmp);
				success = success && nekodata_readVolumeSize(is_tmp, volumeSize_tmp);
				if (!success || totalVolumes_tmp != totalVolumes || volumeSize_tmp != volumeSize || volume_tmp != i + 1)
				{
					return nullptr;
				}
				v_is[i] = is_tmp->createNew();
			}
			ss.str(std::string());
			ss << filepath.substr(0, filepath.size() - nekofs_kNekodata_FileExtension.size());
			ss << u8"." << totalVolumes - 1;
			ss << nekofs_kNekodata_FileExtension;
			auto is_tmp = nativefs->openIStream(ss.str());
			if (!is_tmp || is_tmp->getLength() > volumeSize || is_tmp->getLength() <= nekofs_kNekodata_VolumeFormatSize || is_tmp->seek(-nekofs_kNekodata_FileFooterSize, SeekOrigin::End) != is_tmp->getLength() - nekofs_kNekodata_FileFooterSize)
			{
				return nullptr;
			}
			success = success && nekodata_readVolumeNum(is_tmp, volume_tmp);
			success = success && nekodata_readVolumeNum(is_tmp, totalVolumes_tmp);
			success = success && nekodata_readVolumeSize(is_tmp, volumeSize_tmp);
			if (!success || totalVolumes_tmp != totalVolumes || volumeSize_tmp != volumeSize || volume_tmp != totalVolumes_tmp)
			{
				return nullptr;
			}
			v_is[totalVolumes - 1] = is_tmp->createNew();
		}
		auto nekodatafs = std::shared_ptr<NekodataFileSystem>(new NekodataFileSystem(v_is, volumeSize));
		if (nekodatafs->init())
		{
			return nekodatafs;
		}
		return nullptr;
	}
	bool NekodataFileSystem::verify()
	{
		for (const auto& item : rawFiles_)
		{
			if (item.second.second.getOriginalSize() > 0)
			{
				auto file = openFileInternal(item.first);
				if (!file || !verifySHA256(file->openRawIStream(), item.second.second.getSHA256()))
				{
					return false;
				}
			}
		}
		return true;
	}
	int64_t NekodataFileSystem::getVolumeSzie() const
	{
		return volumeSize_;
	}
	int64_t NekodataFileSystem::getVolumeDataSzie() const
	{
		return volumeSize_ - nekofs_kNekodata_VolumeFormatSize;
	}
	bool NekodataFileSystem::init()
	{
		if (v_is_.empty())
		{
			return false;
		}
		bool success = true;
		int64_t totalSize = (v_is_.size() - 1) * (volumeSize_ - nekofs_kNekodata_VolumeFormatSize);
		totalSize += (v_is_.back()->getLength() - nekofs_kNekodata_VolumeFormatSize);
		auto ris = openRawIStream(0, totalSize);
		int64_t endPos = ris->seek(-8, SeekOrigin::End);
		success = success && endPos >= 0;
		int64_t beginPos;
		success = success && nekodata_readCentralDirectoryPosition(ris, beginPos);
		success = success && beginPos >= 0 && ris->seek(beginPos, SeekOrigin::Begin) == beginPos;
		if (!success)
		{
			return false;
		}
		std::string filepath;
		std::array<uint32_t, 8> sha256 = {};
		int64_t originalSize = 0;
		std::vector<int32_t> blocks;
		while (success && ris->getPosition() < endPos)
		{
			NekodataFileMeta meta;
			blocks.clear();
			success = success && nekodata_readString(ris, filepath);
			success = success && nekodata_readFileSize(ris, originalSize);
			if (success)
			{
				meta.setOriginalSize(originalSize);
			}
			if (success && originalSize > 0)
			{
				int64_t fileBeginPos = 0;
				success = success && nekodata_readPosition(ris, fileBeginPos);
				meta.setBeginPos(fileBeginPos);
				int64_t blockNum;
				success = success && nekodata_readBlockNum(ris, blockNum);
				for (int64_t i = 0; success && i < blockNum; i++)
				{
					int32_t blockSize;
					success = success && nekodata_readBlockSize(ris, blockSize);
					if (success)
					{
						meta.addBlock(blockSize);
					}
				}
				success = success && nekodata_readSHA256(ris, sha256);
				if (success)
				{
					meta.setSHA256(sha256);
				}
			}
			else
			{
				sha256sum hash;
				hash.final();
				meta.setSHA256(hash.readHash());
			}
			if (success)
			{
				rawFiles_[filepath] = std::pair<NekodataFile*, NekodataFileMeta>(nullptr, meta);
			}
		}
		success = ris->getPosition() == endPos;

		return success;
	}
	std::shared_ptr<IStream> NekodataFileSystem::getVolumeIStream(size_t index)
	{
		return v_is_[index]->createNew();
	}
	std::shared_ptr<NekodataRawIStream> NekodataFileSystem::openRawIStream(int64_t beginPos, int64_t length)
	{
		return std::make_shared<NekodataRawIStream>(shared_from_this(), beginPos, length);
	}
	void NekodataFileSystem::weakDeleteCallback(std::weak_ptr<NekodataFileSystem> filesystem, NekodataFile* file)
	{
		auto fsPtr = filesystem.lock();
		if (fsPtr)
		{
			fsPtr->closeFileInternal(file->getFilePath());
		}
		else
		{
			delete file;
		}
	}
	std::shared_ptr<NekodataFile> NekodataFileSystem::openFileInternal(const std::string& filepath)
	{
		std::shared_ptr<NekodataFile> fPtr;
		{
			std::lock_guard lock(mtx_);
			auto it = files_.find(filepath);
			if (it != files_.end())
			{
				fPtr = it->second.lock();
			}
			if (!fPtr)
			{
				auto rit = rawFiles_.find(filepath);
				if (rit != rawFiles_.end())
				{
					if (!rit->second.first)
					{
						rit->second.first = new NekodataFile(shared_from_this(), filepath, &rit->second.second);
					}
				}
				else
				{
					return nullptr;
				}
				fPtr.reset(rit->second.first, std::bind(&NekodataFileSystem::weakDeleteCallback, std::weak_ptr<NekodataFileSystem>(shared_from_this()), std::placeholders::_1));
				files_[filepath] = fPtr;
			}
		}
		return fPtr;
	}
	void NekodataFileSystem::closeFileInternal(const std::string& filepath)
	{
		std::shared_ptr<NekodataFile> fPtr;
		std::lock_guard lock(mtx_);
		auto it = files_.find(filepath);
		if (it != files_.end())
		{
			fPtr = it->second.lock();
		}
		if (!fPtr)
		{
			if (it != files_.end())
			{
				files_.erase(it);
			}
			NekodataFile* rawPtr = nullptr;
			auto rit = rawFiles_.find(filepath);
			if (rit != rawFiles_.end())
			{
				std::swap(rawPtr, rit->second.first);
				delete rawPtr;
			}
		}
	}
}
