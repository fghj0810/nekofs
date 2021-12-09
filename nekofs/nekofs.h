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

	NEKOFS_API NekoFSBool nekofs_native_FileExist(const fschar* filepath);
	NEKOFS_API int64_t nekofs_native_GetFileSize(const fschar* filepath);
	NEKOFS_API NekoFSBool nekofs_native_RemoveFile(const fschar* filepath);
	NEKOFS_API NekoFSBool nekofs_native_RemoveDirectory(const fschar* dirpath);
	NEKOFS_API NekoFSBool nekofs_native_CleanEmptyDirectory(const fschar* dirpath);
	NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(const fschar* filepath);
	NEKOFS_API NekoFSHandle nekofs_native_OpenOStream(const fschar* filepath);

	NEKOFS_API void nekofs_istream_Close(NekoFSHandle isHandle);
	NEKOFS_API int32_t nekofs_istream_Read(NekoFSHandle isHandle, void* buffer, int32_t size);
	NEKOFS_API int64_t nekofs_istream_Seek(NekoFSHandle isHandle, int64_t offset, NekoFSOrigin origin);
	NEKOFS_API int64_t nekofs_istream_GetPosition(NekoFSHandle isHandle);
	NEKOFS_API int64_t nekofs_istream_GetLength(NekoFSHandle isHandle);

	NEKOFS_API void nekofs_ostream_Close(NekoFSHandle oshandle);
	NEKOFS_API int32_t nekofs_ostream_Write(NekoFSHandle oshandle, const void* buffer, int32_t size);
	NEKOFS_API int64_t nekofs_ostream_Seek(NekoFSHandle oshandle, int64_t offset, NekoFSOrigin origin);
	NEKOFS_API int64_t nekofs_ostream_GetPosition(NekoFSHandle oshandle);
	NEKOFS_API int64_t nekofs_ostream_GetLength(NekoFSHandle oshandle);

	NEKOFS_API NekoFSBool nekofs_sha256_sumistream(NekoFSHandle isHandle, uint8_t result[32]);

#ifdef __cplusplus
}
#endif // __cplusplus

