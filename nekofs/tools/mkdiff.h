#pragma once
#include "../common/typedef.h"
#include "../common/rapidjson.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace nekofs {
	class NekodataNativeArchiver;
	class NekodataFileSystem;
}

namespace nekofs::tools {
	class MKDiff final
	{
	public:
		bool exec(const std::string& earlierfile, const std::string& latestfile, const std::string& filepath, int64_t volumeSize);

	private:
		bool diffLayer(std::shared_ptr<NekodataNativeArchiver> archiver, std::shared_ptr<NekodataFileSystem> earlierfs, std::shared_ptr<NekodataFileSystem> latestfs, uint32_t latestVersion);
		std::shared_ptr<JSONStringBuffer> newJsonBuffer();

	private:
		std::vector<std::shared_ptr<JSONStringBuffer>> jsonBuffer_;
	};
}

