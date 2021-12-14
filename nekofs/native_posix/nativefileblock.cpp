#include "nativefileblock.h"
#include "nativefile.h"
#include "../common/utils.h"

#include <cstring>
#include <sstream>

namespace nekofs {
	NativeFileBlock::NativeFileBlock(std::shared_ptr<NativeFile> file, int fd, int64_t offset, int32_t size)
	{
		file_ = file;
		fd_ = fd;
		offset_ = offset;
		size_ = size;
	}
	int32_t NativeFileBlock::read(int64_t pos, void* buffer, int32_t count)
	{
		if (NULL != lpBaseAddress_)
		{
			if (pos < offset_ || pos > offset_ + size_)
			{
				return -1;
			}
			if (pos + count > offset_ + size_)
			{
				count = static_cast<int32_t>(offset_ + size_ - pos);
			}
			if (count > 0)
			{
				uint8_t* writeBuffer = static_cast<uint8_t*>(buffer);
				uint8_t* readBuffer = ((uint8_t*)lpBaseAddress_) - offset_ + pos;
				std::copy(readBuffer, readBuffer + count, writeBuffer);
			}
			return count;
		}
		return -1;
	}
	int64_t NativeFileBlock::getOffset() const
	{
		return offset_;
	}
	int64_t NativeFileBlock::getEndOffset() const
	{
		return offset_ + size_;
	}
	void NativeFileBlock::mmap()
	{
		size_t length = size_;
		off_t offset = offset_;
		lpBaseAddress_ = ::mmap(NULL, length, PROT_READ, MAP_SHARED, fd_, offset);
		if (MAP_FAILED == lpBaseAddress_)
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << "NativeFileBlock::mmap mmap error! filepath = ";
			ss << file_->getFilePath();
			ss << ", offset = ";
			ss << offset_;
			ss << ", size = ";
			ss << size_;
			ss << ", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
		}
	}
	void NativeFileBlock::munmap()
	{
		if (MAP_FAILED != lpBaseAddress_)
		{
			size_t length = size_;
			if (-1 == ::munmap(lpBaseAddress_, length))
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << "NativeFileBlock::munmap munmap error! filepath = ";
				ss << file_->getFilePath();
				ss << ", offset = ";
				ss << offset_;
				ss << ", size = ";
				ss << size_;
				ss << ", err = ";
				ss << errmsg;
				logprint(LogType::Error, ss.str());
			}
			lpBaseAddress_ = MAP_FAILED;
		}
	}
}
