#include "assetmanagerfileistream.h"
#include "assetmanagerfile.h"

namespace nekofs {
	AssetManagerIStream::AssetManagerIStream(std::shared_ptr<AssetManagerFile> file, int64_t fileSize)
	{
		file_ = file;
		fileSize_ = fileSize;
	}

	int32_t AssetManagerIStream::read(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		if (position_ == fileSize_)
		{
			return 0;
		}
		int32_t actulRead = file_->read(position_, buf, size);
		if (actulRead > 0)
		{
			position_ += actulRead;
		}
		return actulRead;
	}
	int64_t AssetManagerIStream::seek(int64_t offset, const SeekOrigin& origin)
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
	int64_t AssetManagerIStream::getPosition() const
	{
		return position_;
	}
	int64_t AssetManagerIStream::getLength() const
	{
		return fileSize_;
	}
	std::shared_ptr<IStream> AssetManagerIStream::createNew()
	{
		return file_->openIStream();
	}
}
