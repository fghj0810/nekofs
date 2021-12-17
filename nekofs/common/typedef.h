#pragma once
#include "nekofs/typedef.h"

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace nekofs {
	enum class SeekOrigin : int32_t
	{
		Begin = NEKOFS_BEGIN,
		Current = NEKOFS_CURRENT,
		End = NEKOFS_END,
	};
	enum class FileSystemType
	{
		Overlay,
		Layer,
		Native,
	};
	enum class FileType : int32_t
	{
		None = NEKOFS_FT_NONE,
		Regular = NEKOFS_FT_REGULAR,
		Directory = NEKOFS_FT_DIRECTORY,
		Unkonwn = NEKOFS_FT_UNKONWN
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
		virtual int32_t write(const void* buf, int32_t size) = 0;
		virtual int64_t seek(int64_t offset, const SeekOrigin& origin) = 0;
		virtual int64_t getPosition() const = 0;
		virtual int64_t getLength() const = 0;
	};
	class FileSystem
	{
	public:
		virtual std::string getCurrentPath() const = 0;
		virtual std::vector<std::string> getAllFiles(const std::string& dirpath) const = 0;
		virtual std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) = 0;
		virtual std::shared_ptr<IStream> openIStream(const std::string& filepath) = 0;
		virtual FileType getFileType(const std::string& path) const = 0;
		virtual int64_t getSize(const std::string& filepath) const = 0;
		virtual FileSystemType getFSType() const = 0;
	};
}

constexpr const char* nekofs_PathSeparator = u8"/";
constexpr const char* nekofs_kURIPrefix_Native = u8"Native:";
constexpr int32_t nekofs_MapBlockSizeBitOffset = 25;
constexpr const char* nekofs_kLayerVersion = u8"version.json";
constexpr const char* nekofs_kLayerVersion_FromVersion = u8"fromVersion";
constexpr const char* nekofs_kLayerVersion_Version = u8"version";
constexpr const char* nekofs_kLayerFiles = u8"files.json";
constexpr const char* nekofs_kLayerFiles_Files = u8"files";
constexpr const char* nekofs_kLayerFiles_FilesVersion = u8"version";
constexpr const char* nekofs_kLayerFiles_FilesSHA256 = u8"sha256";
constexpr const char* nekofs_kLayerFiles_FilesSize = u8"size";
constexpr const char* nekofs_kLayerFiles_Deletes = u8"deletes";
constexpr int32_t nekofs_MapBlockSize = 1 << nekofs_MapBlockSizeBitOffset;
