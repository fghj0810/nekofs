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
	NEKOFS_API void* nekofs_Alloc(uint32_t size);
	NEKOFS_API void nekofs_Free(void* ptr);

	NEKOFS_API NekoFSFileType nekofs_native_GetFileType(const char* u8path);
	NEKOFS_API int64_t nekofs_native_GetFileSize(const char* u8filepath);
	NEKOFS_API NekoFSBool nekofs_native_RemoveFile(const char* u8filepath);
	NEKOFS_API NekoFSBool nekofs_native_RemoveDirectory(const char* u8dirpath);
	NEKOFS_API NekoFSBool nekofs_native_CleanEmptyDirectory(const char* u8dirpath);
	NEKOFS_API NekoFSHandle nekofs_native_OpenIStream(const char* u8filepath);
	NEKOFS_API NekoFSHandle nekofs_native_OpenOStream(const char* u8filepath);
	NEKOFS_API int32_t nekofs_native_GetAllFiles(const char* u8dirpath, char** u8jsonPtr);

	NEKOFS_API void nekofs_istream_Close(NekoFSHandle isHandle);
	NEKOFS_API int32_t nekofs_istream_Read(NekoFSHandle isHandle, void* buffer, int32_t size);
	NEKOFS_API int64_t nekofs_istream_Seek(NekoFSHandle isHandle, int64_t offset, NekoFSOrigin origin);
	NEKOFS_API int64_t nekofs_istream_GetPosition(NekoFSHandle isHandle);
	NEKOFS_API int64_t nekofs_istream_GetLength(NekoFSHandle isHandle);

	NEKOFS_API void nekofs_ostream_Close(NekoFSHandle oshandle);
	NEKOFS_API int32_t nekofs_ostream_Read(NekoFSHandle osHandle, void* buffer, int32_t size);
	NEKOFS_API int32_t nekofs_ostream_Write(NekoFSHandle oshandle, const void* buffer, int32_t size);
	NEKOFS_API int64_t nekofs_ostream_Seek(NekoFSHandle oshandle, int64_t offset, NekoFSOrigin origin);
	NEKOFS_API int64_t nekofs_ostream_GetPosition(NekoFSHandle oshandle);
	NEKOFS_API int64_t nekofs_ostream_GetLength(NekoFSHandle oshandle);

	NEKOFS_API NekoFSBool nekofs_sha256_sumistream32(NekoFSHandle isHandle, uint32_t result[8]);
	NEKOFS_API NekoFSHandle nekofs_nekodata_CreateFromNative(const char* u8filepath);
	NEKOFS_API NekoFSBool nekofs_nekodata_Verify(NekoFSHandle fsHandle);

	NEKOFS_API void nekofs_filesystem_Close(NekoFSHandle fsHandle);
	NEKOFS_API NekoFSFileType nekofs_filesystem_GetFileType(NekoFSHandle fsHandle, const char* u8filepath);
	NEKOFS_API NekoFSHandle nekofs_filesystem_OpenIStream(NekoFSHandle fsHandle, const char* u8filepath);
	NEKOFS_API int32_t nekofs_filesystem_GetAllFiles(NekoFSHandle fsHandle, const char* u8dirpath, char** u8jsonPtr);

	NEKOFS_API NekoFSHandle nekofs_overlay_Create();
	NEKOFS_API int32_t nekofs_overlay_GetLayerVersion(NekoFSHandle olfsHandle, char** u8jsonPtr);
	NEKOFS_API int32_t nekofs_overlay_GetLayerFiles(NekoFSHandle olfsHandle, char** u8jsonPtr);
	NEKOFS_API int32_t nekofs_overlay_GetFileURI(NekoFSHandle olfsHandle, const char* u8filepath, char** u8pathPtr);
	NEKOFS_API NekoFSBool nekofs_overlay_AddLayerFromNaitve(NekoFSHandle olfsHandle, const char* u8dirpath);
	NEKOFS_API NekoFSBool nekofs_overlay_AddLayer(NekoFSHandle olfsHandle, NekoFSHandle fsHandle, const char* u8dirpath);
	NEKOFS_API NekoFSBool nekofs_overlay_RefreshFileList(NekoFSHandle olfsHandle);

#ifdef ANDROID
	NEKOFS_API NekoFSHandle nekofs_assetmanager_OpenIStream(const char* u8filepath);
	NEKOFS_API NekoFSHandle nekofs_nekodata_CreateFromAssetManager(const char* u8filepath);
	NEKOFS_API NekoFSBool nekofs_overlay_AddLayerFromAssetManager(NekoFSHandle olfsHandle, const char* u8dirpath);
#endif

#ifdef NEKOFS_TOOLS
	NEKOFS_API NekoFSBool nekofs_tools_prepare(const char* u8path, const char* u8versionpath, uint32_t offset);
	NEKOFS_API NekoFSBool nekofs_tools_pack(const char* u8dirpath, const char* u8filepath, int64_t volumeSize);
	NEKOFS_API NekoFSBool nekofs_tools_unpack(const char* u8filepath, const char* u8dirpath);
	NEKOFS_API NekoFSBool nekofs_tools_mkldiff(const char* u8earlierfile, const char* u8latestfile, const char* u8filepath, int64_t volumeSize);
	NEKOFS_API NekoFSBool nekofs_tools_mergeToNekodata(const char* u8outpath, int64_t volumeSize, const char** u8filepaths, int32_t filenum, NekoFSBool verify);
	NEKOFS_API NekoFSBool nekofs_tools_mergeToDir(const char* u8outpath, int64_t volumeSize, const char** u8filepaths, int32_t filenum, NekoFSBool verify);
#endif // NEKOFS_TOOLS

#ifdef __cplusplus
}
#endif // __cplusplus

