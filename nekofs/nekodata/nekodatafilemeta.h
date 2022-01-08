#pragma once

#include "../common/typedef.h"

#include <cstdint>
#include <array>
#include <vector>

namespace nekofs {
	class NekodataFileMeta final
	{
	public:
		void setBeginPos(int64_t beginPos);
		int64_t getBeginPos() const;
		void setSHA256(const std::array<uint32_t, 8>& sha256);
		const std::array<uint32_t, 8>& getSHA256() const;
		void setCompressedSize(int64_t compressedSize);
		int64_t getCompressedSize() const;
		void setOriginalSize(int64_t originalSize);
		int64_t getOriginalSize() const;
		void addBlock(int32_t blockSize);
		const std::vector<std::pair<int64_t, int32_t>>& getBlocks() const;

	private:
		std::array<uint32_t, 8> sha256_ = {};
		int64_t beginPos_ = 0;
		int64_t compressedSize_ = 0;
		int64_t originalSize_ = 0;
		std::vector<std::pair<int64_t, int32_t>> blocks_;
	};
}
