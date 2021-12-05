#include "nativefileostream.h"
#include "nativefile.h"
#include "../common/utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace nekofs {
	NativeOStream::NativeOStream(std::shared_ptr<NativeFile> file, int fd)
	{
		file_ = file;
		fd_ = fd;
	}


	int32_t NativeOStream::write(void* buf, int32_t size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		size_t count = size;
		ssize_t ret = ::write(fd_, buf, count);
		if (-1 == ret)
		{
			std::stringstream ss;
			ss << "NativeOStream::write write error ! filepath = ";
			ss << file_->getFilePath();
			ss << ", err = ";
			ss << std::strerror(errno);
			logprint(LogType::Error, ss.str());
		}
		return ret;
	}
	int64_t NativeOStream::seek(int64_t offset, const SeekOrigin& origin)
	{
		off_t value = offset;
		switch (origin)
		{
		case SeekOrigin::Begin:
			value = ::lseek(fd_, value, SEEK_SET);
			break;
		case SeekOrigin::Current:
			value = ::lseek(fd_, value, SEEK_CUR);
			break;
		case SeekOrigin::End:
			value = ::lseek(fd_, value, SEEK_END);
			break;
		default:
			value = -1;
			break;
		}
		if (-1 == value)
		{
			if (origin != SeekOrigin::Unknown)
			{
				std::stringstream ss;
				ss << "NativeOStream::seek lseek error ! filepath = ";
				ss << file_->getFilePath();
				ss << ", err = ";
				ss << std::strerror(errno);
				logprint(LogType::Error, ss.str());
			}
		}
		return value;
	}
	int64_t NativeOStream::getPosition() const
	{
		off_t ret = ::lseek(fd_, 0, SEEK_CUR);
		if (-1 == ret)
		{
			std::stringstream ss;
			ss << "NativeOStream::getPosition lseek error ! filepath = ";
			ss << file_->getFilePath();
			ss << ", err = ";
			ss << std::strerror(errno);
			logprint(LogType::Error, ss.str());
		}
		return ret;
	}
	int64_t NativeOStream::getLength() const
	{
		struct stat info;
		if (-1 == ::fstat(fd_, &info))
		{
			std::stringstream ss;
			ss << "NativeOStream::getLength fstat error ! filepath = ";
			ss << file_->getFilePath();
			ss << ", err = ";
			ss << std::strerror(errno);
			logprint(LogType::Error, ss.str());
			return -1;
		}
		return info.st_size;
	}
}
