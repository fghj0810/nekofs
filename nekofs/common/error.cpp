#include "error.h"


namespace nekofs {
	FSException::FSException(FSError errCode) :runtime_error(nullptr)
	{
		errCode_ = errCode;
	}
	FSError FSException::getErrCode() const
	{
		return errCode_;
	}
}
