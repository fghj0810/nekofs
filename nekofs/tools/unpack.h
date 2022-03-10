#pragma once
#include "../common/typedef.h"

#include <cstdint>
#include <memory>

namespace nekofs {
	class LayerFilesMeta;
}

namespace nekofs::tools {
	class Unpack final
	{
	public:
		static bool exec(const std::string& filepath, const std::string& outpath);

	private:
		static bool unpackLayer(std::shared_ptr<FileSystem> fs, const LayerFilesMeta* lfm , const std::string& outpath, const std::string& progressInfo);
		static bool unpackNormal(std::shared_ptr<FileSystem> fs, const std::string& outpath);
		static bool unpackOneFile(std::shared_ptr<IStream> is, const std::string& outfilepath);
	};
}

