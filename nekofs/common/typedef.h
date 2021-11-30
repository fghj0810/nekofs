#pragma once
#include "../typedef.h"

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#ifdef _WIN32
typedef std::wstring fsstring;
#else
typedef std::string fsstring;
#endif // _WIN32

namespace nekofs {
	enum class LogType : int32_t
	{
		Unknown,
		Info = NEKOFS_LOGINFO,
		Warning = NEKOFS_LOGWARN,
		Error = NEKOFS_LOGERR
	};
	enum class SeekOrigin : int32_t
	{
		Unknown,
		Begin,
		Current,
		End,
	};
	enum class FileSystemType
	{
		Layer,
		Native,
	};

	class FileHandle;
	class IStream;
	class OStream;
	class FileSystem;

	class FileHandle
	{
	public:
		virtual void print() const = 0;
	};

	class IStream {
	public:
		virtual int32_t read(void* buf, int32_t size) = 0;
		virtual int64_t seek(int64_t offset, const SeekOrigin& origin) = 0;
		virtual int64_t getPosition() const = 0;
		virtual int64_t getLength() const = 0;
	};
	class OStream {
	public:
		virtual int32_t write(void* buf, int32_t size) = 0;
		virtual int64_t seek(int64_t offset, const SeekOrigin& origin) = 0;
		virtual int64_t getPosition() const = 0;
		virtual int64_t getLength() const = 0;
	};
	class FileSystem
	{
	public:
		virtual fsstring getCurrentPath() const = 0;
		virtual std::vector<fsstring> getAllFiles(const fsstring& dirpath) const = 0;
		virtual std::unique_ptr<FileHandle> getFileHandle(const fsstring& filepath) = 0;
		virtual std::shared_ptr<IStream> openIStream(const fsstring& filepath) = 0;
		virtual bool fileExist(const fsstring& filepath) const = 0;
		virtual bool dirExist(const fsstring& dirpath) const = 0;
		virtual int64_t getSize(const fsstring& filepath) const = 0;
		virtual FileSystemType getFSType() const = 0;
	};
}

#ifdef _WIN32
constexpr int32_t nekofs_MapBlockSizeBitOffset = 26;
#else
constexpr int32_t nekofs_MapBlockSizeBitOffset = 25;
#endif
constexpr int32_t nekofs_MapBlockSize = 1 << nekofs_MapBlockSizeBitOffset;
