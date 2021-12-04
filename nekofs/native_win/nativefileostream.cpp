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
		if (INVALID_HANDLE_VALUE != fd_)
		{
			DWORD ret = 0;
			if (FALSE == WriteFile(fd_, buf, size, &ret, NULL))
			{
				ret = -1;
				DWORD err = GetLastError();
				LPWSTR msgBuffer = NULL;
				if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
				{
					std::wstringstream ss;
					ss << L"NativeOStream::write WriteFile error ! filepath = ";
					ss << file_->getFilePath();
					ss << L", err = ";
					ss << msgBuffer;
					LocalFree(msgBuffer);
					logprint(LogType::Error, ss.str());
				};
			}
			return ret;
		}
		return -1;
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
				DWORD err = GetLastError();
				LPWSTR msgBuffer = NULL;
				if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
				{
					std::wstringstream ss;
					ss << L"NativeOStream::seek SetFilePointerEx error ! filepath = ";
					ss << file_->getFilePath();
					ss << L", err = ";
					ss << msgBuffer;
					LocalFree(msgBuffer);
					logprint(LogType::Error, ss.str());
				};
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
			DWORD err = GetLastError();
			LPWSTR msgBuffer = NULL;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
			{
				std::wstringstream ss;
				ss << L"NativeOStream::getPosition SetFilePointerEx error ! filepath = ";
				ss << file_->getFilePath();
				ss << L", err = ";
				ss << msgBuffer;
				LocalFree(msgBuffer);
				logprint(LogType::Error, ss.str());
			};
			return -1;
		}
		return position.QuadPart;
	}
	int64_t NativeOStream::getLength() const
	{
		LARGE_INTEGER length;
		if (FALSE == GetFileSizeEx(fd_, &length))
		{
			DWORD err = GetLastError();
			LPWSTR msgBuffer = NULL;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msgBuffer, 0, NULL) > 0)
			{
				std::wstringstream ss;
				ss << L"NativeOStream::getLength GetFileSizeEx error ! filepath = ";
				ss << file_->getFilePath();
				ss << L", err = ";
				ss << msgBuffer;
				LocalFree(msgBuffer);
				logprint(LogType::Error, ss.str());
			};
			return -1;
		}
		return length.QuadPart;
	}
}
