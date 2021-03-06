#pragma once
#include "../common/typedef.h"

#include <cstdint>
#include <memory>

namespace nekofs {
	class NekodataArchiver;
}

namespace nekofs::tools {
	class Pack final
	{
	public:
		static bool exec(const std::string& dirpath, const std::string& outpath, int64_t volumeSize);

	private:
		static bool packDir(std::shared_ptr<nekofs::NekodataArchiver> archiver, const std::string& dirpath);
	};
}

