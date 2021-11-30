#include "nekofs.h"
#include "common/env.h"
#ifdef _WIN32
#include "native_win/nativefilesystem.h"
#endif // _WIN32
#include "common/utils.h"


#include <cstdlib>
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

NEKOFS_API int32_t nekofs_native_FileExist(fschar* filepath)
{
	return nekofs::env::getInstance().getNativeFileSystem()->fileExist(filepath) ? 1 : 0;
}

NEKOFS_API int64_t nekofs_native_GetFileSize(fschar* filepath)
{
	return nekofs::env::getInstance().getNativeFileSystem()->getSize(filepath);
}

NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(fschar* filepath)
{
	NekoFSHandle handle = 0;
	auto stream = nekofs::env::getInstance().getNativeFileSystem()->openIStream(filepath);
	if (stream)
	{
		handle = nekofs::env::getInstance().genId();
		std::lock_guard<std::mutex> lock(g_mtx_istreams_);
		g_istreams_[handle] = stream;
	}
	return handle;
}

NEKOFS_API NekoFSHandle nekofs_native_OpenOStream(fschar* filepath)
{
	NekoFSHandle handle = 0;
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
	std::lock_guard<std::mutex> lock(g_mtx_istreams_);
	auto it = g_istreams_.find(handle);
	if (it != g_istreams_.end())
	{
		g_istreams_.erase(it);
	}
}

NEKOFS_API int32_t nekofs_istream_Read(NekoFSHandle handle, void* buffer, int32_t size)
{
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
		return stream->read(buffer, size);
	}
	return -1;
}

NEKOFS_API int64_t nekofs_istream_Seek(NekoFSHandle handle, int64_t offset, int32_t origin)
{
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
	std::lock_guard<std::mutex> lock(g_mtx_ostreams_);
	auto it = g_ostreams_.find(handle);
	if (it != g_ostreams_.end())
	{
		g_ostreams_.erase(it);
	}
}

NEKOFS_API int32_t nekofs_ostream_Write(NekoFSHandle handle, void* buffer, int32_t size)
{
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
		return stream->write(buffer, size);
	}
	return -1;
}

NEKOFS_API int64_t nekofs_ostream_Seek(NekoFSHandle handle, int64_t offset, int32_t origin)
{
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
