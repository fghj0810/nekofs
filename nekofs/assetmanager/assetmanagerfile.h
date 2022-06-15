#pragma once

#include "../common/typedef.h"

#include <android/asset_manager.h>
#include <cstdint>
#include <memory>
#include <map>
#include <mutex>

namespace nekofs {
	class AssetManagerIStream;

	class AssetManagerFile final: public std::enable_shared_from_this<AssetManagerFile>
	{
	private:
		AssetManagerFile(const AssetManagerFile&) = delete;
		AssetManagerFile(const AssetManagerFile&&) = delete;
		AssetManagerFile& operator=(const AssetManagerFile&) = delete;
		AssetManagerFile& operator=(const AssetManagerFile&&) = delete;

	public:
		AssetManagerFile(const std::string& filepath);
		const std::string& getFilePath() const;
		std::shared_ptr<AssetManagerIStream> openIStream();
		int32_t read(int64_t pos, void* buf, int32_t size);

	private:
		static void weakReadDeleteCallback(std::weak_ptr<AssetManagerFile> file, AssetManagerIStream* istream);
		void openReadFdInternal();
		void closeReadFdInternal();

	private:
		std::string filepath_;
		AAsset* asset_ = NULL;
		int64_t readFileSize_ = 0;
		int32_t readStreamCount_ = 0;
		int64_t rawReadOffset_ = 0;
		std::recursive_mutex mtx_;
	};
}
