﻿#include "nativefileistream.h"
#include "nativefileblock.h"
#include "nativefile.h"

namespace nekofs {
	NativeIStream::NativeIStream(std::shared_ptr<NativeFile> file, int64_t fileSize)
	{
		file_ = file;
		fileSize_ = fileSize;
	}

	int32_t NativeIStream::read(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		int32_t size_tmp = size;
		uint8_t* buffer = static_cast<uint8_t*>(buf);
		int32_t actulRead = 0;
		do
		{
			actulRead = readInternal(buffer, size_tmp);
			if (actulRead > 0)
			{
				size_tmp -= actulRead;
				buffer += actulRead;
			}
		} while (actulRead > 0 && size_tmp > 0);
		return size - size_tmp;
	}
	int64_t NativeIStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		switch (origin)
		{
		case SeekOrigin::Begin:
			if (offset >= 0 && offset <= fileSize_)
			{
				position_ = offset;
			}
			else
			{
				return -1;
			}
			break;
		case SeekOrigin::Current:
			if (position_ + offset >= 0 && position_ + offset <= fileSize_)
			{
				position_ += offset;
			}
			else
			{
				return -1;
			}
			break;
		case SeekOrigin::End:
			if (offset <= 0 && fileSize_ + offset >= 0)
			{
				position_ = offset + fileSize_;
			}
			else
			{
				return -1;
			}
			break;
		default:
			break;
		}
		return position_;
	}
	int64_t NativeIStream::getPosition() const
	{
		return position_;
	}
	int64_t NativeIStream::getLength() const
	{
		return fileSize_;
	}
	int32_t NativeIStream::readInternal(void* buf, int32_t size)
	{
		if (size == 0)
		{
			return 0;
		}
		if (position_ == fileSize_)
		{
			return 0;
		}
		int32_t actulRead = prepareBlock()->read(position_, buf, size);
		if (actulRead > 0)
		{
			position_ += actulRead;
		}
		return actulRead;
	}
	std::shared_ptr<NativeFileBlock> NativeIStream::prepareBlock()
	{
		bool useCurrent = (block_ != nullptr && position_ >= block_->getOffset() && position_ < block_->getEndOffset());
		if (!useCurrent)
		{
			block_ = file_->openBlockInternal(position_);
		}
		return block_;
	}
}
