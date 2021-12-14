#include "nativefileostream.h"
#include "nativefile.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NativeOStream::NativeOStream(std::shared_ptr<NativeFile> file, HANDLE fd)
	{
		file_ = file;
		fd_ = fd;
	}


	int32_t NativeOStream::write(const void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		DWORD ret = 0;
		if (FALSE == WriteFile(fd_, buf, size, &ret, NULL))
		{
			ret = -1;
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << u8"NativeOStream::write WriteFile error ! filepath = ";
			ss << file_->getFilePath();
			ss << u8", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
		}
		return ret;
	}
	int64_t NativeOStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		LARGE_INTEGER distanceToMove;
		LARGE_INTEGER position;
		distanceToMove.QuadPart = offset;
		position.QuadPart = 0;
		bool success = true;
		switch (origin)
		{
		case SeekOrigin::Begin:
			success = (TRUE == SetFilePointerEx(fd_, distanceToMove, &position, FILE_BEGIN));
			break;
		case SeekOrigin::Current:
			success = (TRUE == SetFilePointerEx(fd_, distanceToMove, &position, FILE_CURRENT));
			break;
		case SeekOrigin::End:
			success = (TRUE == SetFilePointerEx(fd_, distanceToMove, &position, FILE_END));
			break;
		}
		if (!success)
		{
			if (origin != SeekOrigin::Unknown)
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeOStream::seek SetFilePointerEx error ! filepath = ";
				ss << file_->getFilePath();
				ss << u8", err = ";
				ss << errmsg;
				logprint(LogType::Error, ss.str());
			}
			return -1;
		}
		return position.QuadPart;
	}
	int64_t NativeOStream::getPosition() const
	{
		LARGE_INTEGER distanceToMove;
		LARGE_INTEGER position;
		distanceToMove.QuadPart = 0;
		position.QuadPart = 0;
		if (FALSE == SetFilePointerEx(fd_, distanceToMove, &position, FILE_CURRENT))
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << u8"NativeOStream::getPosition SetFilePointerEx error ! filepath = ";
			ss << file_->getFilePath();
			ss << u8", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
			return -1;
		}
		return position.QuadPart;
	}
	int64_t NativeOStream::getLength() const
	{
		LARGE_INTEGER length;
		if (FALSE == GetFileSizeEx(fd_, &length))
		{
			auto errmsg = getSysErrMsg();
			std::stringstream ss;
			ss << u8"NativeOStream::getLength GetFileSizeEx error ! filepath = ";
			ss << file_->getFilePath();
			ss << u8", err = ";
			ss << errmsg;
			logprint(LogType::Error, ss.str());
			return -1;
		}
		return length.QuadPart;
	}
}
