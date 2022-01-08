#include "nekodatafile.h"
#include "nekodatafilemeta.h"
#include "nekodatafilesystem.h"
#include "nekodataistream.h"
#include "../common/env.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NekodataFile::NekodataFile(std::shared_ptr<NekodataFileSystem> fs, const std::string& filepath, const NekodataFileMeta* meta)
	{
		fs_ = fs;
		filepath_ = filepath;
		meta_ = meta;
		auto size = meta_->getBlocks().size();
		std::vector<std::pair<BlockStatus, std::weak_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>>>> tmp(meta_->getBlocks().size(), std::pair<BlockStatus, std::weak_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>>>(BlockStatus::None, std::weak_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>>()));
		blocks_.swap(tmp);
	}
	std::shared_ptr<IStream> NekodataFile::openIStream()
	{
		if (meta_->getCompressedSize() == 0)
		{
			return openRawIStream();
		}
		return std::make_shared<NekodataIStream>(shared_from_this());
	}
	std::shared_ptr<IStream> NekodataFile::openRawIStream()
	{
		return fs_->openRawIStream(meta_->getBeginPos(), meta_->getCompressedSize());
	}
	const std::string& NekodataFile::getFilePath() const
	{
		return filepath_;
	}
	int64_t NekodataFile::getFileSize() const
	{
		return meta_->getOriginalSize();
	}
	int64_t NekodataFile::getFileCompressedSize() const
	{
		return meta_->getCompressedSize();
	}
	std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> NekodataFile::getBlock(int64_t index)
	{
		bool needDecompress = false;
		std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> block = nullptr;
		{
			std::unique_lock lock(mtx_);
			block = blocks_[index].second.lock();
			if (!block)
			{
				if (blocks_[index].first != BlockStatus::Error)
				{
					blocks_[index].first = BlockStatus::None;
					block = env::getInstance().newBufferBlockSize();
					blocks_[index].second = block;
					needDecompress = true;
				}
				else
				{
					return nullptr;
				}
			}
			else
			{
				while (blocks_[index].first != BlockStatus::None)
				{
					cond_.wait(lock);
				}
				if (blocks_[index].first == BlockStatus::Decompressed)
				{
					return block;
				}
				else
				{
					return nullptr;
				}
			}
		}
		if (needDecompress)
		{
			bool success = false;
			auto ris = openRawIStream();
			const auto& blocks = meta_->getBlocks();
			if (ris->seek(blocks[index].first, SeekOrigin::Begin) == blocks[index].first)
			{
				int32_t originalSize = nekofs_kNekoData_LZ4_Buffer_Size;
				if (blocks.size() == index + 1)
				{
					originalSize = static_cast<int32_t>(getFileSize() - nekofs_kNekoData_LZ4_Buffer_Size * index);
				}
				auto buffer = env::getInstance().newBufferCompressSize();

				if (istream_read(ris, buffer->data(), blocks[index].second) == blocks[index].second)
				{
					const int decBytes = LZ4_decompress_safe((char*)buffer->data(), (char*)block->data(), blocks[index].second, originalSize);
					if (decBytes == originalSize)
					{
						success = true;
					}
				}
			}
			std::lock_guard lock(mtx_);
			if (success)
			{
				blocks_[index].first = BlockStatus::Decompressed;
			}
			else
			{
				blocks_[index].first = BlockStatus::Error;
				blocks_[index].second.reset();
				block.reset();
			}
		}
		cond_.notify_all();
		return block;
	}
}
