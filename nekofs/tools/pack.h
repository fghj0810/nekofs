#pragma once
#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <cstdint>
#include <memory>
#include <map>
namespace nekofs {
	class NekodataNativeArchiver;
}

namespace nekofs::tools {
	class Pack final
	{
	public:
		static bool exec(const std::string& dirpath, const std::string& outpath, int64_t volumeSize);

	private:
		static bool packDir(std::shared_ptr<nekofs::NekodataNativeArchiver> archiver, const std::string& dirpath);
	};
}

