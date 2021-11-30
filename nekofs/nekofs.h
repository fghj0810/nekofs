#pragma once
#include "typedef.h"

#ifdef _WIN32
#define NEKOFS_API      __declspec(dllexport)
#else
#define NEKOFS_API      extern __attribute__ ((visibility ("default")))
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	NEKOFS_API void nekofs_SetLogDelegate(logdelegate* delegate);

	NEKOFS_API int32_t nekofs_native_FileExist(fschar* filepath);
	NEKOFS_API int64_t nekofs_native_GetFileSize(fschar* filepath);
	NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(fschar* filepath);
	NEKOFS_API NekoFSHandle nekofs_native_OpenOStream(fschar* filepath);

	NEKOFS_API void nekofs_istream_Close(NekoFSHandle handle);
	NEKOFS_API int32_t nekofs_istream_Read(NekoFSHandle handle, void* buffer, int32_t size);
	NEKOFS_API int64_t nekofs_istream_Seek(NekoFSHandle handle, int64_t offset, int32_t origin);
	NEKOFS_API int64_t nekofs_istream_GetPosition(NekoFSHandle handle);
	NEKOFS_API int64_t nekofs_istream_GetLength(NekoFSHandle handle);

	NEKOFS_API void nekofs_ostream_Close(NekoFSHandle handle);
	NEKOFS_API int32_t nekofs_ostream_Write(NekoFSHandle handle, void* buffer, int32_t size);
	NEKOFS_API int64_t nekofs_ostream_Seek(NekoFSHandle handle, int64_t offset, int32_t origin);
	NEKOFS_API int64_t nekofs_ostream_GetPosition(NekoFSHandle handle);
	NEKOFS_API int64_t nekofs_ostream_GetLength(NekoFSHandle handle);

#ifdef __cplusplus
}
#endif // __cplusplus

