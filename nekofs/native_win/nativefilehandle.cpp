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
		std::wstringstream ss;
		if (file_)
		{
			ss << L"NativeFileHandle::print filepath = ";
			ss << file_->getFilePath();
		}
		else
		{
			ss << L"NativeFileHandle::print no file!";
		}
		logprint(LogType::Info, ss.str());
	}
}
