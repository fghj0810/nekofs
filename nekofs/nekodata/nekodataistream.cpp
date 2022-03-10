#include "nekodataistream.h"
#include "nekodatafilesystem.h"
#include "nekodatafile.h"
#include "../common/env.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NekodataRawIStream::NekodataRawIStream(std::shared_ptr<NekodataFileSystem> fs, int64_t beginPos, int64_t length)
	{
		fs_ = fs;
		beginPos_ = beginPos;
		length_ = length;
	}
	std::shared_ptr<IStream> NekodataRawIStream::prepare()
	{
		bool need = is_ == nullptr || voldataRange.first > (beginPos_ + position_) || voldataRange.second <= (beginPos_ + position_);
		if (need)
		{
			int64_t index = (beginPos_ + position_) / fs_->getVolumeDataSzie();
			is_ = fs_->getVolumeIStream(static_cast<size_t>(index));
			voldataRange.first = index * fs_->getVolumeDataSzie();
			voldataRange.second = voldataRange.first + is_->getLength() - nekofs_kNekodata_VolumeFormatSize;
			int64_t offset = (beginPos_ + position_) - voldataRange.first;
			offset += nekofs_kNekodata_FileHeaderSize;
			is_->seek(offset, SeekOrigin::Begin);
		}
		return is_;
	}
	int32_t NekodataRawIStream::read(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (position_ == length_)
		{
			return 0;
		}
		prepare();
		if (position_ + size > length_)
		{
			size = static_cast<int32_t>(length_ - position_);
		}
		if (beginPos_ + position_ + size > voldataRange.second)
		{
			size = static_cast<int32_t>(voldataRange.second - position_ - beginPos_);
		}
		if (size == 0)
		{
			return 0;
		}
		int32_t actulRead = is_->read(buf, size);
		if (actulRead > 0)
		{
			position_ += actulRead;
		}
		return actulRead;
	}
	int64_t NekodataRawIStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		bool success = true;
		int64_t lastPos = position_;
		switch (origin)
		{
		case SeekOrigin::Begin:
			success = offset >= 0 && offset <= length_;
			if (success)
			{
				position_ = offset;
			}
			break;
		case SeekOrigin::Current:
			success = offset + position_ >= 0 && offset + position_ <= length_;
			if (success)
			{
				position_ = offset + position_;
			}
			break;
		case SeekOrigin::End:
			success = offset <= 0 && offset + length_ >= 0;
			if (success)
			{
				position_ = offset + length_;
			}
			break;
		}
		if (success && position_ != lastPos && is_ != nullptr && (beginPos_ + position_) >= voldataRange.first && (beginPos_ + position_) < voldataRange.second)
		{
			int64_t offset = (beginPos_ + position_) - voldataRange.first;
			offset += nekofs_kNekodata_FileHeaderSize;
			success = is_->seek(offset, SeekOrigin::Begin) == offset;
		}
		if (!success)
		{
			std::stringstream ss;
			ss << u8"NekodataRawIStream::seek error ! offset = ";
			ss << offset;
			ss << u8", origin = ";
			ss << static_cast<int32_t>(origin);
			ss << u8", position = ";
			ss << position_;
			logerr(ss.str());
			return -1;
		}
		return position_;
	}
	int64_t NekodataRawIStream::getPosition() const
	{
		return position_;
	}
	int64_t NekodataRawIStream::getLength() const
	{
		return length_;
	}
	std::shared_ptr<IStream> NekodataRawIStream::createNew()
	{
		return fs_->openRawIStream(beginPos_, length_);
	}


	NekodataIStream::NekodataIStream(std::shared_ptr<NekodataFile> file)
	{
		file_ = file;
	}
	std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> NekodataIStream::prepare()
	{
		bool need = block_ == nullptr || blockBeginPos_ > position_ || blockEndPos_ <= position_;
		if (need)
		{
			int64_t index = position_ / nekofs_kNekoData_LZ4_Buffer_Size;
			block_ = file_->getBlock(index);
			blockBeginPos_ = index * nekofs_kNekoData_LZ4_Buffer_Size;
			blockEndPos_ = std::min(file_->getFileSize(), nekofs_kNekoData_LZ4_Buffer_Size + blockBeginPos_);
		}
		return block_;
	}
	int32_t NekodataIStream::read(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (position_ == getLength())
		{
			return 0;
		}
		prepare();
		if (block_)
		{
			if (size + position_ > blockEndPos_)
			{
				size = static_cast<int32_t>(blockEndPos_ - position_);
			}
			int32_t begin = static_cast<int32_t>(position_ - blockBeginPos_);
			std::copy(block_->data() + begin, block_->data() + (begin + size), static_cast<uint8_t*>(buf));
			position_ += size;
			return size;
		}
		return -1;
	}
	int64_t NekodataIStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		bool success = true;
		int64_t length = getLength();
		int64_t lastPos = position_;
		switch (origin)
		{
		case SeekOrigin::Begin:
			success = offset >= 0 && offset <= length;
			if (success)
			{
				position_ = offset;
			}
			break;
		case SeekOrigin::Current:
			success = offset + position_ >= 0 && offset + position_ <= length;
			if (success)
			{
				position_ = offset + position_;
			}
			break;
		case SeekOrigin::End:
			success = offset <= 0 && offset + length >= 0;
			if (success)
			{
				position_ = offset + length;
			}
			break;
		}
		if (!success)
		{
			std::stringstream ss;
			ss << u8"NekodataIStream::seek error ! offset = ";
			ss << offset;
			ss << u8", origin = ";
			ss << static_cast<int32_t>(origin);
			ss << u8", position = ";
			ss << position_;
			logerr(ss.str());
			return -1;
		}
		return position_;
	}
	int64_t NekodataIStream::getPosition() const
	{
		return position_;
	}
	int64_t NekodataIStream::getLength() const
	{
		return file_->getFileSize();
	}
	std::shared_ptr<IStream> NekodataIStream::createNew()
	{
		return file_->openIStream();
	}
}
