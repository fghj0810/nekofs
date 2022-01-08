#include "nekodataostream.h"
#include "nekodataarchiver.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NekodataOStream::NekodataOStream(std::shared_ptr<NekodataNativeArchiver> archiver, int64_t volumeSize)
	{
		archiver_ = archiver;
		volumeSize_ = volumeSize;
	}
	int32_t NekodataOStream::read(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		prepareVolumeOStream();
		if (!os_)
		{
			return -1;
		}
		else
		{
			auto os_begin = os_->getVolDataBeginPos();
			auto os_pos = os_->getPosition();
			auto needpos = position_ - os_begin + nekofs_kNekodata_FileHeader.size();
			if (needpos != os_pos && os_->seek(needpos, SeekOrigin::Begin) != needpos)
			{
				return -1;
			}
		}
		if (position_ + size > os_->getVolDataEndPos_max())
		{
			size = static_cast<int32_t>(os_->getVolDataEndPos_max() - position_);
		}
		if (size == 0)
		{
			return 0;
		}
		auto readnum = os_->read(buf, size);
		if (readnum > 0)
		{
			position_ += readnum;
		}
		return readnum;
	}
	int32_t NekodataOStream::write(const void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		prepareVolumeOStream();
		if (!os_)
		{
			return -1;
		}
		else
		{
			auto os_begin = os_->getVolDataBeginPos();
			auto os_pos = os_->getPosition();
			auto needpos = position_ - os_begin + nekofs_kNekodata_FileHeader.size();
			if (needpos != os_pos && os_->seek(needpos, SeekOrigin::Begin) != needpos)
			{
				return -1;
			}
		}
		if (position_ + size > os_->getVolDataEndPos_max())
		{
			size = static_cast<int32_t>(os_->getVolDataEndPos_max() - position_);
		}
		if (size == 0)
		{
			return 0;
		}
		auto writenum = os_->write(buf, size);
		if (writenum > 0)
		{
			position_ += writenum;
			length_ = std::max(length_, position_);
		}
		return writenum;
	}
	int64_t NekodataOStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		bool success = true;
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
		if (!success)
		{
			std::stringstream ss;
			ss << u8"NekodataOStream::seek error ! offset = ";
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
	int64_t NekodataOStream::getPosition() const
	{
		return position_;
	}
	int64_t NekodataOStream::getLength() const
	{
		return length_;
	}
	std::shared_ptr<NekodataVolumeOStream> NekodataOStream::prepareVolumeOStream()
	{
		bool useCurrent = (os_ && position_ >= os_->getVolDataBeginPos() && position_ < os_->getVolDataEndPos_max());
		if (!useCurrent)
		{
			os_ = archiver_->getVolumeOStreamByDataPos(position_);
		}
		return os_;
	}


	NekodataVolumeOStream::NekodataVolumeOStream(std::shared_ptr<OStream> os, int64_t volumeSize, int64_t voldataBeginPos)
	{
		os_ = os;
		rawBeginPos_ = os->getPosition();
		volumeSize_ = volumeSize;
		voldataBeginPos_ = voldataBeginPos;
	}
	int64_t NekodataVolumeOStream::getVolDataBeginPos() const
	{
		return voldataBeginPos_;
	}
	int64_t NekodataVolumeOStream::getVolDataEndPos_max() const
	{
		return voldataBeginPos_ + volumeSize_ - 12 - nekofs_kNekodata_FileHeader.size();
	}
	bool NekodataVolumeOStream::fill()
	{
		std::shared_ptr<OStream> ptr = shared_from_this();
		char buffer[8192] = { 0 };
		for (int64_t roundnum = (volumeSize_ - position_) / 8192; roundnum > 0; roundnum--)
		{
			if (8192 != ostream_write(ptr, buffer, 8192))
			{
				return false;
			}
		}
		int32_t size = static_cast<int32_t>(volumeSize_ - position_);
		if (size != ostream_write(ptr, buffer, size))
		{
			return false;
		}
		return true;
	}
	int32_t NekodataVolumeOStream::read(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (position_ + size > length_)
		{
			size = static_cast<int32_t>(length_ - position_);
		}
		if (size == 0)
		{
			return 0;
		}
		auto readnum = os_->read(buf, size);
		if (readnum > 0)
		{
			position_ += readnum;
		}
		return readnum;
	}
	int32_t NekodataVolumeOStream::write(const void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (position_ + size > volumeSize_)
		{
			size = static_cast<int32_t>(volumeSize_ - position_);
		}
		if (size == 0)
		{
			return 0;
		}
		auto writenum = os_->write(buf, size);
		if (writenum > 0)
		{
			position_ += writenum;
			length_ = std::max(length_, position_);
		}
		return writenum;
	}
	int64_t NekodataVolumeOStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		bool success = true;
		int64_t pos = position_;
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
		if (!success)
		{
			std::stringstream ss;
			ss << u8"NekodataOStream::seek error ! offset = ";
			ss << offset;
			ss << u8", origin = ";
			ss << static_cast<int32_t>(origin);
			ss << u8", position = ";
			ss << position_;
			logerr(ss.str());
			return -1;
		}
		if (pos != position_)
		{
			if (os_->seek(rawBeginPos_ + position_, SeekOrigin::Begin) < 0)
			{
				position_ = pos;
				return -1;
			}
		}
		return position_;
	}
	int64_t NekodataVolumeOStream::getPosition() const
	{
		return position_;
	}
	int64_t NekodataVolumeOStream::getLength() const
	{
		return length_;
	}
}
