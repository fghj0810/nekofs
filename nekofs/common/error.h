#pragma once
#include "../typedef.h"

#include <stdexcept>

namespace nekofs {
	enum class FSError : int32_t
	{
		Unknown,
		WriteErr = NEKOFS_ERRCODE_WRITEERR,
	};

	class FSException final : public std::runtime_error
	{
	public:
		explicit FSException(FSError errCode);
		FSError getErrCode() const;

	private:
		FSError errCode_ = FSError::Unknown;
	};
}
