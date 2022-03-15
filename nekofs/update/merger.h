#pragma once
#include "../common/typedef.h"
#include "../common/rapidjson.h"
#include "../layer/layerfilesmeta.h"
#include "../nekodata/nekodatafilemeta.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <tuple>
#include <optional>

namespace nekofs {
	class NekodataNativeArchiver;
	class LayerVersionMeta;

	class Merger final
	{
		Merger(const Merger&) = delete;
		Merger(Merger&&) = delete;
		Merger& operator=(const Merger&) = delete;
		Merger& operator=(Merger&&) = delete;
	private:
		struct RawStreamInfo
		{
			std::shared_ptr<FileSystem> fs = nullptr;
			std::shared_ptr<IStream> rawis = nullptr;
			NekodataFileMeta meta;
		};

	public:
		Merger(const std::string& resName, uint32_t baseVersion = 0);
		bool addPatch(std::shared_ptr<FileSystem> fs);
		bool exec(const std::string& outdir, int64_t volumeSize);
		bool exec(std::shared_ptr<NekodataNativeArchiver> archiver);
		bool getProgress(int64_t& complete, int64_t& total);

	private:
		bool prepare(int64_t& total, const std::vector<std::shared_ptr<FileSystem>>& fslist);
		void exec(std::shared_ptr<NekodataNativeArchiver> archiver, const std::vector<std::shared_ptr<FileSystem>>& fslist);
		std::optional<LayerFilesMeta> getLayerFilesMeta(const std::vector<std::shared_ptr<FileSystem>>& fslist);
		std::shared_ptr<IStream> getIStream(const std::vector<std::shared_ptr<FileSystem>>& fslist, const std::string& filepath);
		RawStreamInfo tryGetIStream(const std::vector<std::shared_ptr<FileSystem>>& fslist, const std::string& filepath);
		void addComplete();
		std::shared_ptr<JSONStringBuffer> newJsonBuffer();

	private:
		std::string resName_;
		uint32_t baseVersion_ = 0;
		std::vector<std::tuple<std::shared_ptr<FileSystem>, LayerVersionMeta>> patchfs_;
		std::mutex mtx_;
		int64_t complete_ = 0;
		int64_t total_ = 0;
		std::vector<std::shared_ptr<JSONStringBuffer>> jsonBuffer_;
	};
}

