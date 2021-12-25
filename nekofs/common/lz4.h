#pragma once
#include "typedef.h"

#include <../../thirdparty/lz4/lib/lz4.h>
#include <../../thirdparty/lz4/lib/lz4hc.h>

namespace nekofs {
	constexpr int nekofs_kNekoData_LZ4_Buffer_Lsh = 15;
	constexpr int nekofs_kNekoData_LZ4_Buffer_Size = 1 << nekofs_kNekoData_LZ4_Buffer_Lsh;
	constexpr int nekofs_kNekoData_LZ4_Compress_Buffer_Size = LZ4_COMPRESSBOUND(nekofs_kNekoData_LZ4_Buffer_Size);
}
