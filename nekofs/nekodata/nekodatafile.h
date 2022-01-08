#pragma once

#include "../common/typedef.h"
#include "../common/lz4.h"

#include <cstdint>
#include <array>
#include <string>
#include <memory>
#include <utility>
#include <mutex>
#include <condition_variable>

namespace nekofs {
	class NekodataFileMeta;
	class NekodataFileSystem;
	class NekodataIStream;

	class NekodataFile final : public std::enable_shared_from_this<NekodataFile>
	{
		enum class BlockStatus
		{
			None,
			Decompressed,
			Error
		};
		friend class NekodataIStream;
		NekodataFile(const NekodataFile&) = delete;
		NekodataFile(NekodataFile&&) = delete;
		NekodataFile& operator=(const NekodataFile&) = delete;
		NekodataFile& operator=(NekodataFile&&) = delete;
	public:
		NekodataFile(std::shared_ptr<NekodataFileSystem> fs, const std::string& filepath, const NekodataFileMeta* meta);
		std::shared_ptr<IStream> openIStream();
		std::shared_ptr<IStream> openRawIStream();
		const std::string& getFilePath() const;
		int64_t getFileSize() const;
		int64_t getFileCompressedSize() const;

	private:
		std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> getBlock(int64_t index);

	private:
		std::shared_ptr<NekodataFileSystem> fs_;
		std::string filepath_;
		const NekodataFileMeta* meta_ = nullptr;
		std::vector<std::pair<BlockStatus, std::weak_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>>>> blocks_;
		std::mutex mtx_;
		std::condition_variable cond_;
	};
}
