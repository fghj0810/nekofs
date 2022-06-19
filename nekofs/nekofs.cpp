#include "nekofs/nekofs.h"
#include "common/env.h"
#include "common/utils.h"
#include "common/sha256.h"
#include "common/rapidjson.h"
#ifdef _WIN32
#include "native_win/nativefilesystem.h"
#else
#include "native_posix/nativefilesystem.h"
#endif
#ifdef ANDROID
#include "assetmanager/assetmanagerfilesystem.h"
#endif
#include "layer/overlayfilesystem.h"
#include "nekodata/nekodatafilesystem.h"

#include <cstring>
#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <filesystem>

std::mutex g_mtx_istreams_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::IStream>> g_istreams_;
std::mutex g_mtx_ostreams_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::OStream>> g_ostreams_;
std::mutex g_mtx_fs_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::FileSystem>> g_filesystems_;


// 传入 utf8 字符串，返回utf8字符串
static inline std::string __normalrootpath(const char* u8path)
{
#ifdef _WIN32
	std::filesystem::path p(nekofs::u8_to_u16(u8path));
#else
	std::filesystem::path p(u8path);
#endif
	if (!p.has_root_path())
	{
		return std::string();
	}
#ifdef _WIN32
	return nekofs::u16_to_u8(p.lexically_normal().generic_wstring());
#else
	return p.lexically_normal().generic_string();
#endif
}

// 传入 utf8 字符串，返回utf8字符串
static inline std::string __normalpath(const char* u8path)
{
#ifdef _WIN32
	std::filesystem::path p(nekofs::u8_to_u16(u8path));
#else
	std::filesystem::path p(u8path);
#endif
	if (p.has_root_path())
	{
		return std::string();
	}

#ifdef _WIN32
	return nekofs::u16_to_u8(p.lexically_normal().generic_wstring());
#else
	return p.lexically_normal().generic_string();
#endif
}
static inline bool checkSeekOrigin(const NekoFSOrigin& value)
{
	return (value == NEKOFS_BEGIN) || (value == NEKOFS_CURRENT) || (value == NEKOFS_END);
}
static inline int32_t copy_and_newString(const char* src, size_t size, char** dest)
{
	*dest = nullptr;
	if (size > static_cast<size_t>(0x7FFFFFFF))
	{
		return 0;
	}
	*dest = static_cast<char*>(nekofs_Alloc(static_cast<uint32_t>(size)));
	if (*dest == nullptr)
	{
		return 0;
	}
	std::copy(src, src + size, *dest);
	return static_cast<int32_t>(size);
}

NEKOFS_API void nekofs_SetLogDelegate(logdelegate* delegate)
{
	nekofs::env::getInstance().setLogDelegate(delegate);
}

NEKOFS_API void* nekofs_Alloc(uint32_t size)
{
	return ::malloc(size);
}

NEKOFS_API void nekofs_Free(void* ptr)
{
	::free(ptr);
}

NEKOFS_API NekoFSFileType nekofs_native_GetFileType(const char* u8path)
{
	auto path = __normalrootpath(u8path);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return static_cast<NekoFSFileType>(nekofs::env::getInstance().getNativeFileSystem()->getFileType(path));
}

NEKOFS_API int64_t nekofs_native_GetFileSize(const char* u8filepath)
{
	auto path = __normalrootpath(u8filepath);
	if (path.empty())
	{
		return -1;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->getSize(path);
}

NEKOFS_API NekoFSBool nekofs_native_RemoveFile(const char* u8filepath)
{
	auto path = __normalrootpath(u8filepath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->removeFile(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_native_RemoveDirectory(const char* u8dirpath)
{
	auto path = __normalrootpath(u8dirpath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->removeDirectories(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_native_CleanEmptyDirectory(const char* u8dirpath)
{
	auto path = __normalrootpath(u8dirpath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->cleanEmptyDirectories(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(const char* u8filepath)
{
	auto path = __normalrootpath(u8filepath);
	if (path.empty())
	{
		return INVALID_NEKOFSHANDLE;
	}
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	auto stream = nekofs::env::getInstance().getNativeFileSystem()->openIStream(path);
	if (stream)
	{
		handle = nekofs::env::getInstance().genId();
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		g_istreams_[handle] = stream;
	}
	return handle;
}

NEKOFS_API NekoFSHandle nekofs_native_OpenOStream(const char* u8filepath)
{
	auto path = __normalrootpath(u8filepath);
	if (path.empty())
	{
		return INVALID_NEKOFSHANDLE;
	}
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	auto stream = nekofs::env::getInstance().getNativeFileSystem()->openOStream(path);
	if (stream)
	{
		handle = nekofs::env::getInstance().genId();
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		g_ostreams_[handle] = stream;
	}
	return handle;
}

NEKOFS_API int32_t nekofs_native_GetAllFiles(const char* u8dirpath, char** u8jsonPtr)
{
	*u8jsonPtr = nullptr;
	auto path = __normalrootpath(u8dirpath);
	if (path.empty())
	{
	}
	auto allfiles = nekofs::env::getInstance().getNativeFileSystem()->getAllFiles(path);
	std::sort(allfiles.begin(), allfiles.end());

	nekofs::JSONStringBuffer jsonStrBuffer;
	nekofs::JSONStringWriter writer(jsonStrBuffer);
	nekofs::JSONDocument d(nekofs::rapidjson::kArrayType);
	auto& allocator = d.GetAllocator();

	for (const auto& item : allfiles)
	{
		d.PushBack(nekofs::rapidjson::StringRef(item.c_str()), allocator);
	}

	if (d.Accept(writer))
	{
		return copy_and_newString(jsonStrBuffer.GetString(), jsonStrBuffer.GetSize(), u8jsonPtr);
	}
	return 0;
}

NEKOFS_API void nekofs_istream_Close(NekoFSHandle isHandle)
{
	if (INVALID_NEKOFSHANDLE == isHandle)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(g_mtx_istreams_);
	auto it = g_istreams_.find(isHandle);
	if (it != g_istreams_.end())
	{
		g_istreams_.erase(it);
		nekofs::env::getInstance().ungenId(isHandle);
	}
}

NEKOFS_API int32_t nekofs_istream_Read(NekoFSHandle isHandle, void* buffer, int32_t size)
{
	if (INVALID_NEKOFSHANDLE == isHandle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(isHandle);
		if (it != g_istreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return nekofs::istream_read(stream, buffer, size);
	}
	return -1;
}

NEKOFS_API int64_t nekofs_istream_Seek(NekoFSHandle isHandle, int64_t offset, NekoFSOrigin origin)
{
	if (INVALID_NEKOFSHANDLE == isHandle)
	{
		return -1;
	}
	if (!checkSeekOrigin(origin))
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(isHandle);
		if (it != g_istreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return stream->seek(offset, static_cast<nekofs::SeekOrigin>(origin));
	}
	return -1;
}

NEKOFS_API int64_t nekofs_istream_GetPosition(NekoFSHandle isHandle)
{
	if (INVALID_NEKOFSHANDLE == isHandle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(isHandle);
		if (it != g_istreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return stream->getPosition();
	}
	return -1;
}

NEKOFS_API int64_t nekofs_istream_GetLength(NekoFSHandle isHandle)
{
	if (INVALID_NEKOFSHANDLE == isHandle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(isHandle);
		if (it != g_istreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return stream->getLength();
	}
	return -1;
}

NEKOFS_API void nekofs_ostream_Close(NekoFSHandle oshandle)
{
	if (INVALID_NEKOFSHANDLE == oshandle)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
	auto it = g_ostreams_.find(oshandle);
	if (it != g_ostreams_.end())
	{
		g_ostreams_.erase(it);
		nekofs::env::getInstance().ungenId(oshandle);
	}
}

NEKOFS_API int32_t nekofs_ostream_Write(NekoFSHandle oshandle, const void* buffer, int32_t size)
{
	if (INVALID_NEKOFSHANDLE == oshandle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(oshandle);
		if (it != g_ostreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return nekofs::ostream_write(stream, buffer, size);
	}
	return -1;
}

NEKOFS_API int64_t nekofs_ostream_Seek(NekoFSHandle oshandle, int64_t offset, NekoFSOrigin origin)
{
	if (INVALID_NEKOFSHANDLE == oshandle)
	{
		return -1;
	}
	if (!checkSeekOrigin(origin))
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(oshandle);
		if (it != g_ostreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return stream->seek(offset, static_cast<nekofs::SeekOrigin>(origin));
	}
	return -1;
}

NEKOFS_API int64_t nekofs_ostream_GetPosition(NekoFSHandle oshandle)
{
	if (INVALID_NEKOFSHANDLE == oshandle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(oshandle);
		if (it != g_ostreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return stream->getPosition();
	}
	return -1;
}

NEKOFS_API int64_t nekofs_ostream_GetLength(NekoFSHandle oshandle)
{
	if (INVALID_NEKOFSHANDLE == oshandle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(oshandle);
		if (it != g_ostreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		return stream->getLength();
	}
	return -1;
}

NEKOFS_API NekoFSBool nekofs_sha256_sumistream32(NekoFSHandle isHandle, uint32_t result[8])
{
	::memset(result, 0, 32);
	if (INVALID_NEKOFSHANDLE == isHandle)
	{
		return NEKOFS_FALSE;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(isHandle);
		if (it != g_istreams_.end())
		{
			stream = it->second;
		}
	}
	if (stream)
	{
		const size_t buffer_size = 8 * 1024;
		uint8_t buffer[buffer_size];
		int32_t actualRead = 0;
		nekofs::sha256sum sha256;
		do
		{
			actualRead = nekofs::istream_read(stream, buffer, buffer_size);
			if (actualRead > 0)
			{
				sha256.update(buffer, actualRead);
			}
		} while (actualRead > 0);
		if (actualRead >= 0)
		{
			sha256.final();
			const auto& hashResult = sha256.readHash();
			std::copy(hashResult.begin(), hashResult.end(), result);
			return NEKOFS_TRUE;
		}
	}
	return NEKOFS_FALSE;
}

NEKOFS_API NekoFSHandle nekofs_nekodata_CreateFromNative(const char* u8filepath)
{
	auto path = __normalrootpath(u8filepath);
	if (path.empty())
	{
		return INVALID_NEKOFSHANDLE;
	}
	auto nekodatafs = nekofs::NekodataFileSystem::create(nekofs::env::getInstance().getNativeFileSystem(), path);
	if (nekodatafs)
	{
		auto handle = nekofs::env::getInstance().genId();
		std::lock_guard lock(g_mtx_fs_);
		g_filesystems_[handle] = nekodatafs;
		return handle;
	}
	return INVALID_NEKOFSHANDLE;
}

NEKOFS_API NekoFSBool nekofs_nekodata_Verify(NekoFSHandle fsHandle)
{
	std::shared_ptr<nekofs::NekodataFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(fsHandle);
		if (it != g_filesystems_.end() && it->second->getFSType() == nekofs::FileSystemType::Nekodata)
		{
			fs = std::static_pointer_cast<nekofs::NekodataFileSystem>(it->second);
		}
	}
	if (fs && fs->verify())
	{
		return NEKOFS_TRUE;
	}
	return NEKOFS_FALSE;
}

NEKOFS_API void nekofs_filesystem_Close(NekoFSHandle fsHandle)
{
	if (INVALID_NEKOFSHANDLE == fsHandle)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(g_mtx_fs_);
	auto it = g_filesystems_.find(fsHandle);
	if (it != g_filesystems_.end())
	{
		g_filesystems_.erase(it);
		nekofs::env::getInstance().ungenId(fsHandle);
	}
}

NEKOFS_API NekoFSFileType nekofs_filesystem_GetFileType(NekoFSHandle fsHandle, const char* u8filepath)
{
	auto path = __normalpath(u8filepath);
	if (path.empty())
	{
		return NEKOFS_FT_NONE;
	}
	std::shared_ptr<nekofs::FileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(fsHandle);
		if (it != g_filesystems_.end())
		{
			fs = it->second;
		}
	}
	if (fs)
	{
		return static_cast<NekoFSFileType>(fs->getFileType(path));
	}
	return NEKOFS_FT_NONE;
}

NEKOFS_API NekoFSHandle nekofs_filesystem_OpenIStream(NekoFSHandle fsHandle, const char* u8filepath)
{
	auto path = __normalpath(u8filepath);
	if (path.empty())
	{
		return INVALID_NEKOFSHANDLE;
	}
	std::shared_ptr<nekofs::FileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(fsHandle);
		if (it != g_filesystems_.end())
		{
			fs = it->second;
		}
	}
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	if (fs)
	{
		auto stream = fs->openIStream(path);
		if (stream)
		{
			handle = nekofs::env::getInstance().genId();
			std::lock_guard<std::mutex> lock(g_mtx_istreams_);
			g_istreams_[handle] = stream;
		}
	}
	return handle;
}

NEKOFS_API int32_t nekofs_filesystem_GetAllFiles(NekoFSHandle fsHandle, const char* u8dirpath, char** u8jsonPtr)
{
	*u8jsonPtr = nullptr;
	auto path = __normalpath(u8dirpath);
	if (path.empty() && u8dirpath != nullptr && ::strcmp(u8dirpath, u8"") != 0)
	{
		return 0;
	}
	std::shared_ptr<const nekofs::FileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(fsHandle);
		if (it != g_filesystems_.end())
		{
			fs = it->second;
		}
	}
	if (fs)
	{
		auto allfiles = fs->getAllFiles(path);
		std::sort(allfiles.begin(), allfiles.end());

		nekofs::JSONStringBuffer jsonStrBuffer;
		nekofs::JSONStringWriter writer(jsonStrBuffer);
		nekofs::JSONDocument d(nekofs::rapidjson::kArrayType);
		auto& allocator = d.GetAllocator();

		for (const auto& item : allfiles)
		{
			d.PushBack(nekofs::rapidjson::StringRef(item.c_str()), allocator);
		}

		if (d.Accept(writer))
		{
			return copy_and_newString(jsonStrBuffer.GetString(), jsonStrBuffer.GetSize(), u8jsonPtr);
		}
	}
	return 0;
}

NEKOFS_API NekoFSHandle nekofs_overlay_Create()
{
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	auto overlayfs = std::make_shared<nekofs::OverlayFileSystem>();
	handle = nekofs::env::getInstance().genId();
	std::lock_guard<std::mutex> lock(g_mtx_fs_);
	g_filesystems_[handle] = overlayfs;
	return handle;
}

NEKOFS_API int32_t nekofs_overlay_GetLayerVersion(NekoFSHandle olfsHandle, char** u8jsonPtr)
{
	*u8jsonPtr = nullptr;
	std::shared_ptr<const nekofs::OverlayFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(olfsHandle);
		if (it != g_filesystems_.end())
		{
			if (fs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				fs = std::static_pointer_cast<nekofs::OverlayFileSystem>(it->second);
			}
		}
	}
	if (fs)
	{
		auto lvm = fs->getVersion();
		if (!lvm)
		{
			return 0;
		}
		nekofs::JSONStringBuffer jsonStrBuffer;
		nekofs::JSONStringWriter writer(jsonStrBuffer);
		nekofs::JSONDocument d(nekofs::rapidjson::kObjectType);

		lvm->save(&d, d.GetAllocator());

		if (d.Accept(writer))
		{
			return copy_and_newString(jsonStrBuffer.GetString(), jsonStrBuffer.GetSize(), u8jsonPtr);
		}
	}
	return 0;
}

NEKOFS_API int32_t nekofs_overlay_GetLayerFiles(NekoFSHandle olfsHandle, char** u8jsonPtr)
{
	*u8jsonPtr = nullptr;
	std::shared_ptr<const nekofs::OverlayFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(olfsHandle);
		if (it != g_filesystems_.end())
		{
			if (fs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				fs = std::static_pointer_cast<nekofs::OverlayFileSystem>(it->second);
			}
		}
	}
	if (fs)
	{
		auto lfm = fs->getFiles();
		if (!lfm)
		{
			return 0;
		}
		nekofs::JSONStringBuffer jsonStrBuffer;
		nekofs::JSONStringWriter writer(jsonStrBuffer);
		nekofs::JSONDocument d(nekofs::rapidjson::kObjectType);

		lfm->save(&d, d.GetAllocator());

		if (d.Accept(writer))
		{
			return copy_and_newString(jsonStrBuffer.GetString(), jsonStrBuffer.GetSize(), u8jsonPtr);
		}
	}
	return 0;
}

NEKOFS_API int32_t nekofs_overlay_GetFileURI(NekoFSHandle olfsHandle, const char* u8filepath, char** u8pathPtr)
{
	*u8pathPtr = nullptr;
	auto path = __normalpath(u8filepath);
	if (path.empty())
	{
		return 0;
	}
	std::shared_ptr<const nekofs::OverlayFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(olfsHandle);
		if (it != g_filesystems_.end())
		{
			if (fs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				fs = std::static_pointer_cast<nekofs::OverlayFileSystem>(it->second);
			}
		}
	}
	if (fs)
	{
		auto uri = fs->getFileURI(path);
		if (uri.empty())
		{
			return 0;
		}
		return copy_and_newString(uri.c_str(), uri.size(), u8pathPtr);
	}
	return 0;
}

NEKOFS_API NekoFSBool nekofs_overlay_AddLayerFromNaitve(NekoFSHandle olfsHandle, const char* u8dirpath)
{
	auto path = __normalrootpath(u8dirpath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	std::shared_ptr<nekofs::OverlayFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(olfsHandle);
		if (it != g_filesystems_.end())
		{
			if (fs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				fs = std::static_pointer_cast<nekofs::OverlayFileSystem>(it->second);
			}
		}
	}
	if (fs)
	{
		return fs->addLayer(nekofs::env::getInstance().getNativeFileSystem(), path) ? NEKOFS_TRUE : NEKOFS_FALSE;
	}
	return NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_overlay_AddLayer(NekoFSHandle olfsHandle, NekoFSHandle fsHandle, const char* u8dirpath)
{
	auto path = __normalpath(u8dirpath);
	std::shared_ptr<nekofs::OverlayFileSystem> olfs;
	std::shared_ptr<nekofs::FileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto olit = g_filesystems_.find(olfsHandle);
		if (olit != g_filesystems_.end())
		{
			if (olfs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				olfs = std::static_pointer_cast<nekofs::OverlayFileSystem>(olit->second);
			}
		}
		auto it = g_filesystems_.find(fsHandle);
		if (it != g_filesystems_.end())
		{
			fs = it->second;
		}
	}
	if (olfs && fs)
	{
		return olfs->addLayer(fs, path) ? NEKOFS_TRUE : NEKOFS_FALSE;
	}
	return NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_overlay_RefreshFileList(NekoFSHandle olfsHandle)
{
	std::shared_ptr<nekofs::OverlayFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(olfsHandle);
		if (it != g_filesystems_.end())
		{
			if (fs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				fs = std::static_pointer_cast<nekofs::OverlayFileSystem>(it->second);
			}
		}
	}
	if (fs)
	{
		return fs->refreshFileList() ? NEKOFS_TRUE : NEKOFS_FALSE;
	}
	return NEKOFS_FALSE;
}

#ifdef ANDROID
NEKOFS_API NekoFSHandle nekofs_assetmanager_OpenIStream(const char* u8filepath)
{
	auto path = __normalpath(u8filepath);
	if (path.empty())
	{
		return INVALID_NEKOFSHANDLE;
	}
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	auto stream = nekofs::env::getInstance().getAssetManagerFileSystem()->openIStream(path);
	if (stream)
	{
		handle = nekofs::env::getInstance().genId();
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		g_istreams_[handle] = stream;
	}
	return handle;
}

NEKOFS_API NekoFSHandle nekofs_nekodata_CreateFromAssetManager(const char* u8filepath)
{
	auto path = __normalpath(u8filepath);
	if (path.empty())
	{
		return INVALID_NEKOFSHANDLE;
	}
	auto nekodatafs = nekofs::NekodataFileSystem::create(nekofs::env::getInstance().getAssetManagerFileSystem(), path);
	if (nekodatafs)
	{
		auto handle = nekofs::env::getInstance().genId();
		std::lock_guard lock(g_mtx_fs_);
		g_filesystems_[handle] = nekodatafs;
		return handle;
	}
	return INVALID_NEKOFSHANDLE;
}

NEKOFS_API NekoFSBool nekofs_overlay_AddLayerFromAssetManager(NekoFSHandle olfsHandle, const char* u8dirpath)
{
	auto path = __normalpath(u8dirpath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	std::shared_ptr<nekofs::OverlayFileSystem> fs;
	{
		std::lock_guard<std::mutex> lock(g_mtx_fs_);
		auto it = g_filesystems_.find(olfsHandle);
		if (it != g_filesystems_.end())
		{
			if (fs->getFSType() == nekofs::FileSystemType::Overlay)
			{
				fs = std::static_pointer_cast<nekofs::OverlayFileSystem>(it->second);
			}
		}
	}
	if (fs)
	{
		return fs->addLayer(nekofs::env::getInstance().getAssetManagerFileSystem(), path) ? NEKOFS_TRUE : NEKOFS_FALSE;
	}
	return NEKOFS_FALSE;
}
#endif // ANDROID

#ifdef NEKOFS_TOOLS
#include "tools/prepare.h"
#include "tools/pack.h"
#include "tools/unpack.h"
#include "tools/mkdiff.h"
#include "tools/merge.h"

NEKOFS_API NekoFSBool nekofs_tools_prepare(const char* u8path, const char* u8versionpath, uint32_t offset)
{
	auto path = __normalrootpath(u8path);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	auto vpath = __normalrootpath(u8versionpath);
	if (vpath.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::tools::PrePare::exec(path, vpath, offset) ? NEKOFS_TRUE : NEKOFS_FALSE;
}
NEKOFS_API NekoFSBool nekofs_tools_pack(const char* u8dirpath, const char* u8filepath, int64_t volumeSize)
{
	if (volumeSize > nekofs_kNekodata_MaxVolumeSize || volumeSize <= 1024)
	{
		return NEKOFS_FALSE;
	}
	auto dpath = __normalrootpath(u8dirpath);
	if (dpath.empty())
	{
		return NEKOFS_FALSE;
	}
	auto fpath = __normalrootpath(u8filepath);
	if (fpath.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::tools::Pack::exec(dpath, fpath, volumeSize) ? NEKOFS_TRUE : NEKOFS_FALSE;
}
NEKOFS_API NekoFSBool nekofs_tools_unpack(const char* u8filepath, const char* u8dirpath)
{
	auto fpath = __normalrootpath(u8filepath);
	if (fpath.empty())
	{
		return NEKOFS_FALSE;
	}
	auto dpath = __normalrootpath(u8dirpath);
	if (dpath.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::tools::Unpack::exec(fpath, dpath) ? NEKOFS_TRUE : NEKOFS_FALSE;
}
NEKOFS_API NekoFSBool nekofs_tools_mkldiff(const char* u8earlierfile, const char* u8latestfile, const char* u8filepath, int64_t volumeSize)
{
	if (volumeSize > nekofs_kNekodata_MaxVolumeSize || volumeSize <= 1024)
	{
		return NEKOFS_FALSE;
	}
	auto filepath = __normalrootpath(u8filepath);
	if (filepath.empty())
	{
		return NEKOFS_FALSE;
	}
	auto earlierfile = __normalrootpath(u8earlierfile);
	if (earlierfile.empty())
	{
		return NEKOFS_FALSE;
	}
	auto latestfile = __normalrootpath(u8latestfile);
	if (latestfile.empty())
	{
		return NEKOFS_FALSE;
	}
	nekofs::tools::MKDiff mkdiff;
	return mkdiff.exec(earlierfile, latestfile, filepath, volumeSize) ? NEKOFS_TRUE : NEKOFS_FALSE;
}
NEKOFS_API NekoFSBool nekofs_tools_mergeToNekodata(const char* u8outpath, int64_t volumeSize, const char** u8filepaths, int32_t filenum, NekoFSBool verify)
{
	if (volumeSize > nekofs_kNekodata_MaxVolumeSize || volumeSize <= 1024)
	{
		return NEKOFS_FALSE;
	}
	auto outpath = __normalrootpath(u8outpath);
	if (outpath.empty())
	{
		return NEKOFS_FALSE;
	}
	if (filenum <= 0)
	{
		return NEKOFS_FALSE;
	}
	std::vector<std::string> patchfiles;
	for (int32_t i = 0; i < filenum; i++)
	{
		auto path = __normalrootpath(u8filepaths[i]);
		if (path.empty())
		{
			return NEKOFS_FALSE;
		}
		patchfiles.push_back(path);
	}
	return nekofs::tools::Merge::execNekodata(outpath, volumeSize, patchfiles, verify == NEKOFS_TRUE) ? NEKOFS_TRUE : NEKOFS_FALSE;
}
NEKOFS_API NekoFSBool nekofs_tools_mergeToDir(const char* u8outpath, int64_t volumeSize, const char** u8filepaths, int32_t filenum, NekoFSBool verify)
{
	if (volumeSize > nekofs_kNekodata_MaxVolumeSize || volumeSize <= 1024)
	{
		return NEKOFS_FALSE;
	}
	auto outpath = __normalrootpath(u8outpath);
	if (outpath.empty())
	{
		return NEKOFS_FALSE;
	}
	if (filenum <= 0)
	{
		return NEKOFS_FALSE;
	}
	std::vector<std::string> patchfiles;
	for (int32_t i = 0; i < filenum; i++)
	{
		auto path = __normalrootpath(u8filepaths[i]);
		if (path.empty())
		{
			return NEKOFS_FALSE;
		}
		patchfiles.push_back(path);
	}
	return nekofs::tools::Merge::execDir(outpath, volumeSize, patchfiles, verify == NEKOFS_TRUE) ? NEKOFS_TRUE : NEKOFS_FALSE;
}
#endif // NEKOFS_TOOLS
