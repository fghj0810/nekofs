#include "assetmanagerfile.h"
#include "assetmanagerfileistream.h"
#include "assetmanagerfilesystem.h"
#include "../common/utils.h"
#include "../common/env.h"

#include <cstring>
#include <sstream>
#include <functional>

namespace nekofs {
	AssetManagerFile::AssetManagerFile(const std::string& filepath)
	{
		filepath_ = filepath;
	}
	const std::string& AssetManagerFile::getFilePath() const
	{
		return filepath_;
	}
	std::shared_ptr<AssetManagerIStream> AssetManagerFile::openIStream()
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		openReadFdInternal();
		if (NULL == asset_)
		{
			return nullptr;
		}
		readStreamCount_++;
		std::shared_ptr<AssetManagerIStream> isPtr;
		isPtr.reset(new AssetManagerIStream(shared_from_this(), readFileSize_), std::bind(&AssetManagerFile::weakReadDeleteCallback, std::weak_ptr<AssetManagerFile>(shared_from_this()), std::placeholders::_1));
		return isPtr;
	}
	int32_t AssetManagerFile::read(int64_t pos, void* buf, int32_t size)
	{
		if (size < 0 || pos < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		size = std::min(size, 32 * 1024);  // 一次读取少一点，并发能高一些
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (NULL == asset_)
		{
			return -1;
		}
		if (pos != rawReadOffset_)
		{
			off_t newPos = ::AAsset_seek(asset_, pos, SEEK_SET);
			if (newPos == ((off_t)-1))
			{
				std::stringstream ss;
				ss << u8"AssetManagerFile::read AAsset_seek error! filepath = ";
				ss << filepath_;
				logerr(ss.str());
			}
			if (newPos > 0)
			{
				rawReadOffset_ = newPos;
			}
			if (newPos != pos)
			{
				return -1;
			}
		}
		int32_t actulRead = ::AAsset_read(asset_, buf, size);
		if (actulRead < 0)
		{
			std::stringstream ss;
			ss << u8"AssetManagerFile::read AAsset_read error! filepath = ";
			ss << filepath_;
			logerr(ss.str());
		}
		if (actulRead > 0)
		{
			rawReadOffset_ += actulRead;
		}
		return actulRead;
	}


	void AssetManagerFile::weakReadDeleteCallback(std::weak_ptr<AssetManagerFile> file, AssetManagerIStream* istream)
	{
		auto fp = file.lock();
		std::lock_guard<std::recursive_mutex> lock(fp->mtx_);
		fp->readStreamCount_--;
		if (fp->readStreamCount_ == 0)
		{
			fp->closeReadFdInternal();
		}
		delete istream;
	}
	void AssetManagerFile::openReadFdInternal()
	{
		if (NULL == asset_)
		{
			auto amPtr = env::getAssetManagerPtr();
			if (amPtr == nullptr)
			{
				std::stringstream ss;
				ss << u8"AssetManagerFile::openReadFdInternal amPtr is nullptr. filepath = ";
				ss << filepath_;
				logerr(ss.str());
				return;
			}
			asset_ = ::AAssetManager_open(amPtr, filepath_.c_str(), AASSET_MODE_RANDOM);
			if (NULL == asset_)
			{
				std::stringstream ss;
				ss << u8"AssetManagerFile::openReadFdInternal AAssetManager_open error! filepath = ";
				ss << filepath_;
				logerr(ss.str());
			}
			else
			{
				off_t fileSize = ::AAsset_getLength64(asset_);
				readFileSize_ = fileSize;
			}
		}
	}
	void AssetManagerFile::closeReadFdInternal()
	{
		if (NULL != asset_)
		{
			::AAsset_close(asset_);
			asset_ = NULL;
		}
		rawReadOffset_ = 0;
		readFileSize_ = 0;
	}
}
