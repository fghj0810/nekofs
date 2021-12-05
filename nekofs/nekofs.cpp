#include "nekofs.h"
#include "common/env.h"
#ifdef _WIN32
#include "native_win/nativefilesystem.h"
#else
#include "native_posix/nativefilesystem.h"
#endif
#include "common/utils.h"


#include <memory>
#include <mutex>
#include <unordered_map>

std::mutex g_mtx_istreams_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::IStream>> g_istreams_;
std::mutex g_mtx_ostreams_;
std::unordered_map<NekoFSHandle, std::shared_ptr<nekofs::OStream>> g_ostreams_;

NEKOFS_API void nekofs_SetLogDelegate(logdelegate* delegate)
{
	nekofs::env::getInstance().setLogDelegate(delegate);
}

NEKOFS_API NekoFSBool nekofs_native_FileExist(const fschar* filepath)
{
	return nekofs::env::getInstance().getNativeFileSystem()->fileExist(filepath) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API int64_t nekofs_native_GetFileSize(const fschar* filepath)
{
	return nekofs::env::getInstance().getNativeFileSystem()->getSize(filepath);
}

NEKOFS_API NekoFSBool nekofs_native_RemoveFile(const fschar* filepath)
{
	return nekofs::env::getInstance().getNativeFileSystem()->removeFile(filepath) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSBool nekofs_native_RemoveDirectory(const fschar* dirpath)
{
	return nekofs::env::getInstance().getNativeFileSystem()->removeDirectories(dirpath) ? NEKOFS_TRUE : NEKOFS_FALSE;
}

NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(const fschar* filepath)
{
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	auto stream = nekofs::env::getInstance().getNativeFileSystem()->openIStream(filepath);
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
	NekoFSHandle handle = INVALID_NEKOFSHANDLE;
	auto stream = nekofs::env::getInstance().getNativeFileSystem()->openOStream(filepath);
	if (stream)
	{
		handle = nekofs::env::getInstance().genId();
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		g_ostreams_[handle] = stream;
	}
	return handle;
}

NEKOFS_API void nekofs_istream_Close(NekoFSHandle handle)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(g_mtx_istreams_);
	auto it = g_istreams_.find(handle);
	if (it != g_istreams_.end())
	{
		g_istreams_.erase(it);
	}
}

NEKOFS_API int32_t nekofs_istream_Read(NekoFSHandle handle, void* buffer, int32_t size)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(handle);
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

NEKOFS_API int64_t nekofs_istream_Seek(NekoFSHandle handle, int64_t offset, NekoFSOrigin origin)
{
	if (INVALID_NEKOFSHANDLE == handle)
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
		auto it = g_istreams_.find(handle);
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

NEKOFS_API int64_t nekofs_istream_GetPosition(NekoFSHandle handle)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(handle);
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

NEKOFS_API int64_t nekofs_istream_GetLength(NekoFSHandle handle)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::IStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		auto it = g_istreams_.find(handle);
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

NEKOFS_API void nekofs_ostream_Close(NekoFSHandle handle)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
	auto it = g_ostreams_.find(handle);
	if (it != g_ostreams_.end())
	{
		g_ostreams_.erase(it);
	}
}

NEKOFS_API int32_t nekofs_ostream_Write(NekoFSHandle handle, void* buffer, int32_t size)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(handle);
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

NEKOFS_API int64_t nekofs_ostream_Seek(NekoFSHandle handle, int64_t offset, NekoFSOrigin origin)
{
	if (INVALID_NEKOFSHANDLE == handle)
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
		auto it = g_ostreams_.find(handle);
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

NEKOFS_API int64_t nekofs_ostream_GetPosition(NekoFSHandle handle)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(handle);
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

NEKOFS_API int64_t nekofs_ostream_GetLength(NekoFSHandle handle)
{
	if (INVALID_NEKOFSHANDLE == handle)
	{
		return -1;
	}
	std::shared_ptr<nekofs::OStream> stream;
	{
		std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
		auto it = g_ostreams_.find(handle);
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
