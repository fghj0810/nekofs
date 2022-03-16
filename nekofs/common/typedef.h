#pragma once
#include "nekofs/typedef.h"

#include <cstdint>
#include <string>
#include <string_view>
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
		Nekodata,
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
		virtual ~FileHandle() = default;
		virtual void print() const = 0;
	};

	class IStream {
	public:
		virtual int32_t read(void* buf, int32_t size) = 0;
		virtual int64_t seek(int64_t offset, const SeekOrigin& origin) = 0;
		virtual int64_t getPosition() const = 0;
		virtual int64_t getLength() const = 0;
		virtual std::shared_ptr<IStream> createNew() = 0;
	};
	class OStream {
	public:
		virtual int32_t read(void* buf, int32_t size) = 0;
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
constexpr const char* nekofs_kURIPrefix_Nekodata = u8"Nekodata:";

constexpr int32_t nekofs_MapBlockSizeBitOffset = 25;
constexpr int32_t nekofs_MapBlockSize = 1 << nekofs_MapBlockSizeBitOffset;

constexpr const char* nekofs_kLayerVersion = u8"version.json";
constexpr const char* nekofs_kLayerVersion_Name = u8"name";
constexpr const char* nekofs_kLayerVersion_FromVersion = u8"fromVersion";
constexpr const char* nekofs_kLayerVersion_Version = u8"version";
constexpr const char* nekofs_kLayerVersion_VersionServers = u8"versionServers";
constexpr const char* nekofs_kLayerVersion_DownloadServers = u8"downloadServers";
constexpr const char* nekofs_kLayerVersion_SubResources = u8"subResources";
constexpr const char* nekofs_kLayerVersion_Relocate = u8"relocate";
constexpr const char* nekofs_kLayerVersion_Require = u8"require";

constexpr const char* nekofs_kLayerFiles = u8"files.json";
constexpr const char* nekofs_kLayerFiles_Files = u8"files";
constexpr const char* nekofs_kLayerFiles_FilesVersion = u8"version";
constexpr const char* nekofs_kLayerFiles_FilesSHA256 = u8"sha256";
constexpr const char* nekofs_kLayerFiles_FilesSize = u8"size";
constexpr const char* nekofs_kLayerFiles_Nekodatas = u8"nekodatas";
constexpr const char* nekofs_kLayerFiles_Deletes = u8"deletes";

constexpr const std::string_view nekofs_kNekodata_FileExtension = u8".nekodata";
constexpr const std::string_view nekofs_kNekodata_FileHeader = u8".nekodata";
constexpr const int32_t nekofs_kNekodata_FileHeaderSize = static_cast<int32_t>(nekofs_kNekodata_FileHeader.size());
constexpr const int32_t nekofs_kNekodata_FileFooterSize = 12;
constexpr const int32_t nekofs_kNekodata_VolumeFormatSize = nekofs_kNekodata_FileHeaderSize + nekofs_kNekodata_FileFooterSize;
constexpr const int64_t nekofs_kNekodata_MaxVolumeSize = 1LL << (20 + 31);
constexpr const int64_t nekofs_kNekodata_DefalutVolumeSize = 1LL << 20;
