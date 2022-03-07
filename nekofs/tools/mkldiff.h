#pragma once
#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"
#include "../common/rapidjson.h"

#include <cstdint>
#include <memory>
#include <map>
#include <optional>

namespace nekofs {
	class NekodataNativeArchiver;
	class NekodataFileSystem;
}

namespace nekofs::tools {
	class MKLDiff final
	{
	public:
		static bool exec(const std::string& filepath, const std::string& earlierfile, const std::string& latestfile);

	private:
		static bool diffLayer(std::shared_ptr<NekodataNativeArchiver> archiver, std::shared_ptr<NekodataFileSystem> earlierfs, std::shared_ptr<NekodataFileSystem> latestfs, uint32_t latestVersion);
	};
}

