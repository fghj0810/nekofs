#include "nativefileblock.h"
#include "nativefile.h"
#include "nativefilesystem.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NativeFileBlock::NativeFileBlock(std::shared_ptr<NativeFile> file, HANDLE readMapFd, int64_t offset, int32_t size)
	{
		file_ = file;
		readMapFd_ = readMapFd;
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
		DWORD dwFileOffsetHigh = offset_ >> 32;
		DWORD dwFileOffsetLow = offset_ & 0x0FFFFFFFF;
		SIZE_T dwNumberOfBytesToMap = size_;
		lpBaseAddress_ = MapViewOfFile(readMapFd_, FILE_MAP_READ, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
		if (NULL == lpBaseAddress_)
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << u8"NativeFileBlock::mmap MapViewOfFile error! filepath = ";
			ss << file_->getFilePath();
			ss << u8", offset = ";
			ss << offset_;
			ss << u8", size = ";
			ss << size_;
			ss << u8", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
		}
	}
	void NativeFileBlock::munmap()
	{
		if (NULL != lpBaseAddress_)
		{
			if (FALSE == UnmapViewOfFile(lpBaseAddress_))
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeFileBlock::munmap UnmapViewOfFile error! filepath = ";
				ss << file_->getFilePath();
				ss << u8", offset = ";
				ss << offset_;
				ss << u8", size = ";
				ss << size_;
				ss << u8", err = ";
				ss << errmsg;
				logprint(LogType::Error, ss.str());
			}
			lpBaseAddress_ = NULL;
		}
	}
}
