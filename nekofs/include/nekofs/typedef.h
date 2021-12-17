#pragma once
#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif // __cplusplus

	typedef int32_t NEKOFSLogLevel;
	typedef int32_t NekoFSOrigin;
	typedef int32_t NekoFSBool;
	typedef int32_t NekoFSFileType;
	typedef int32_t NekoFSHandle;
	typedef void logdelegate(NEKOFSLogLevel level, const char* u8message);

#ifdef __cplusplus
}
#endif // __cplusplus

#define NEKOFS_BEGIN    ((NekoFSOrigin)1)
#define NEKOFS_CURRENT  ((NekoFSOrigin)2)
#define NEKOFS_END      ((NekoFSOrigin)3)

#define NEKOFS_LOGINFO  ((NEKOFSLogLevel)1)
#define NEKOFS_LOGWARN  ((NEKOFSLogLevel)2)
#define NEKOFS_LOGERR   ((NEKOFSLogLevel)3)

#define NEKOFS_FT_NONE       ((NekoFSFileType)0)
#define NEKOFS_FT_REGULAR    ((NekoFSFileType)1)
#define NEKOFS_FT_DIRECTORY  ((NekoFSFileType)1)
#define NEKOFS_FT_UNKONWN    ((NekoFSFileType)-1)

#define INVALID_NEKOFSHANDLE ((NekoFSHandle)-1)
#define NEKOFS_TRUE ((NekoFSBool)1)
#define NEKOFS_FALSE ((NekoFSBool)0)

#define NEKOFS_ERRCODE_WRITEERR (1)
