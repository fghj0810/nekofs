#include "nekodatafilemeta.h"

namespace nekofs {
	void NekodataFileMeta::setBeginPos(int64_t beginPos)
	{
		beginPos_ = beginPos;
	}
	int64_t NekodataFileMeta::getBeginPos() const
	{
		return beginPos_;
	}
	void NekodataFileMeta::setSHA256(const std::array<uint32_t, 8>& sha256)
	{
		sha256_ = sha256;
	}
	const std::array<uint32_t, 8>& NekodataFileMeta::getSHA256() const
	{
		return sha256_;
	}
	void NekodataFileMeta::setCompressedSize(int64_t compressedSize)
	{
		compressedSize_ = compressedSize;
	}
	int64_t NekodataFileMeta::getCompressedSize() const
	{
		return compressedSize_;
	}
	void NekodataFileMeta::setOriginalSize(int64_t originalSize)
	{
		originalSize_ = originalSize;
	}
	int64_t NekodataFileMeta::getOriginalSize() const
	{
		return originalSize_;
	}
	void NekodataFileMeta::addBlock(int32_t blockSize)
	{
		blocks_.push_back(std::pair<int64_t, int32_t>(compressedSize_, blockSize));
		compressedSize_ += blockSize;
	}
	const std::vector<std::pair<int64_t, int32_t>>& NekodataFileMeta::getBlocks() const
	{
		return blocks_;
	}
}
