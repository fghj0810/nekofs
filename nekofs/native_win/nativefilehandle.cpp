#include "nativefilehandle.h"
#include "nativefile.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	NativeFileHandle::NativeFileHandle(std::shared_ptr<NativeFile> fPtr)
	{
		file_ = fPtr;
	}
	void NativeFileHandle::print() const
	{
		std::stringstream ss;
		if (file_)
		{
			ss << u8"NativeFileHandle::print filepath = ";
			ss << file_->getFilePath();
		}
		else
		{
			ss << u8"NativeFileHandle::print no file!";
		}
		loginfo(ss.str());
	}
}
