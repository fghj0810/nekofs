#include "nekofs.h"
#include "common/env.h"
#ifdef _WIN32
#include "native_win/nativefilesystem.h"
#else
#include "native_posix/nativefilesystem.h"
#endif
#include "common/utils.h"
#include "common/sha256.h"

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <filesystem>

std::mutex g_mtx_istreams_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::IStream>> g_istreams_;
std::mutex g_mtx_ostreams_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::OStream>> g_ostreams_;

static inline fsstring __normalpath(const fschar* path)
{
	std::filesystem::path p(path);
	if (!p.has_root_path())
	{
		return fsstring();
	}
#ifdef _WIN32
	return p.lexically_normal().generic_wstring();
#else
	return p.lexically_normal().generic_string();
#endif
}

NEKOFS_API void nekofs_SetLogDelegate(logdelegate* delegate)
{
	nekofs::env::getInstance().setLogDelegate(delegate);
}

NEKOFS_API NekoFSBool nekofs_native_FileExist(const fschar* filepath)
{
	auto path = __normalpath(filepath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->fileExist(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API int64_t nekofs_native_GetFileSize(const fschar* filepath)
{
	auto path = __normalpath(filepath);
	if (path.empty())
	{
		return -1;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->getSize(path);
}

NEKOFS_API NekoFSBool nekofs_native_RemoveFile(const fschar* filepath)
{
	auto path = __normalpath(filepath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->removeFile(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_native_RemoveDirectory(const fschar* dirpath)
{
	auto path = __normalpath(dirpath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->removeDirectories(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_native_CleanEmptyDirectory(const fschar* dirpath)
{
	auto path = __normalpath(dirpath);
	if (path.empty())
	{
		return NEKOFS_FALSE;
	}
	return nekofs::env::getInstance().getNativeFileSystem()->cleanEmptyDirectories(path) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(const fschar* filepath)
{
	auto path = __normalpath(filepath);
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

NEKOFS_API NekoFSHandle nekofs_native_OpenOStream(const fschar* filepath)
{
	auto path = __normalpath(filepath);
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
	nekofs::SeekOrigin seekOrigin = nekofs::Int2SeekOrigin(origin);
	if (seekOrigin == nekofs::SeekOrigin::Unknown)
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
		return stream->seek(offset, seekOrigin);
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
	nekofs::SeekOrigin seekOrigin = nekofs::Int2SeekOrigin(origin);
	if (seekOrigin == nekofs::SeekOrigin::Unknown)
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
		return stream->seek(offset, seekOrigin);
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

NEKOFS_API NekoFSBool nekofs_sha256_sumistream(NekoFSHandle isHandle, uint8_t result[32])
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
		stream->seek(0, nekofs::SeekOrigin::Begin);
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
			sha256.final(nullptr, 0, result);
			return NEKOFS_TRUE;
		}
	}
	return NEKOFS_FALSE;
}
