#include "nativefileblock.h"
#include "nativefile.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NativeFileBlock::NativeFileBlock(std::shared_ptr<NativeFile> file, int64_t offset, int32_t size)
	{
		file_ = file;
		offset_ = offset;
		size_ = size;
		lpBaseAddress_ = MapViewOfFile(file->getMapHandle(), FILE_MAP_READ, offset >> 32, offset & 0x0FFFFFFFF, size);
		if (NULL == lpBaseAddress_)
		{
			DWORD err = GetLastError();
			LPWSTR msgBuffer = NULL;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
			{
				std::wstringstream ss;
				ss << L"NativeFileBlock::NativeFileBlock MapViewOfFile error! filepath = ";
				ss << file_->getFilePath();
				ss << L", offset = ";
				ss << offset_;
				ss << L", size = ";
				ss << size_;
				ss << L", err = ";
				ss << msgBuffer;
				LocalFree(msgBuffer);
				logprint(LogType::Error, ss.str());
			};
		}
	}
	NativeFileBlock::~NativeFileBlock()
	{
		if (NULL != lpBaseAddress_)
		{
			if (FALSE == UnmapViewOfFile(lpBaseAddress_))
			{
				DWORD err = GetLastError();
				LPWSTR msgBuffer = NULL;
				if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
				{
					std::wstringstream ss;
					ss << L"NativeFileBlock::NativeFileBlock MapViewOfFile error! filepath = ";
					ss << file_->getFilePath();
					ss << L", offset = ";
					ss << offset_;
					ss << L", size = ";
					ss << size_;
					ss << L", err = ";
					ss << msgBuffer;
					LocalFree(msgBuffer);
					logprint(LogType::Error, ss.str());
				};
			}
			lpBaseAddress_ = NULL;
		}
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
}
