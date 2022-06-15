#include "assetmanagerfilehandle.h"
#include "assetmanagerfile.h"
#include "../common/utils.h"

#include <sstream>

namespace nekofs {
	AssetManagerFileHandle::AssetManagerFileHandle(std::shared_ptr<AssetManagerFile> fPtr)
	{
		file_ = fPtr;
	}
	void AssetManagerFileHandle::print() const
	{
		std::stringstream ss;
		if (file_)
		{
			ss << u8"AssetManagerFileHandle::print filepath = ";
			ss << file_->getFilePath();
		}
		else
		{
			ss << u8"AssetManagerFileHandle::print no file!";
		}
		loginfo(ss.str());
	}
}
