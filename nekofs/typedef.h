﻿#pragma once
#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif // __cplusplus

	typedef int32_t NekoFSOrigin;
	typedef uint32_t NekoFSBool;
	typedef int32_t NekoFSHandle;
	typedef void logdelegate(int32_t level, const char* u8message);

#ifdef __cplusplus
}
#endif // __cplusplus

#define NEKOFS_BEGIN    ((NekoFSOrigin)1)
#define NEKOFS_CURRENT  ((NekoFSOrigin)2)
#define NEKOFS_END      ((NekoFSOrigin)3)

#define NEKOFS_LOGINFO  (1)
#define NEKOFS_LOGWARN  (2)
#define NEKOFS_LOGERR   (3)

#define INVALID_NEKOFSHANDLE ((NekoFSHandle)-1)
#define NEKOFS_TRUE ((NekoFSBool)1)
#define NEKOFS_FALSE ((NekoFSBool)0)

#define NEKOFS_ERRCODE_WRITEERR (1)
