#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"
#include "../common/rapidjson.h"

#include <cstdint>
#include <array>
#include <memory>
#include <map>
#include <set>
#include <optional>

namespace nekofs {
	class LayerFilesMeta final
	{
	public:
		class FileMeta;

	public:
		static std::optional<LayerFilesMeta> load(std::shared_ptr<IStream> is);
		static std::optional<LayerFilesMeta> load(const JSONValue* jsondoc);
		static LayerFilesMeta merge(const std::vector<LayerFilesMeta>& lfms, uint32_t baseVersion);
		static LayerFilesMeta makediff(const LayerFilesMeta& lfms_earlier, const LayerFilesMeta& lfms_latest, const uint32_t latestVersion);
		bool save(std::shared_ptr<OStream> os) const;
		bool save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator) const;
		void setFileMeta(const std::string& filename, const LayerFilesMeta::FileMeta& meta);
		std::optional<LayerFilesMeta::FileMeta> getFileMeta(const std::string& filename) const;
		const std::map<std::string, LayerFilesMeta::FileMeta>& getFiles() const;
		void addNekodata(const std::string& filename);
		const std::set<std::string>& getNekodatas() const;
		void setDeleteVersion(const std::string& filename, const uint32_t& version);
		std::optional<uint32_t> getDeleteVersion(const std::string& filename) const;
		const std::map<std::string, uint32_t>& getDeletes() const;
		void purgeFile(const std::string& filename);

	private:
		std::map<std::string, LayerFilesMeta::FileMeta> files_;
		std::set<std::string> nekodatas_;
		std::map<std::string, uint32_t> deletes_;
	};


	class LayerFilesMeta::FileMeta final
	{
	public:
		void setVersion(const uint32_t& version);
		uint32_t getVersion() const;
		void setSHA256(const std::array<uint32_t, 8>& sha256);
		const std::array<uint32_t, 8>& getSHA256() const;
		void setSize(const int64_t& size);
		int64_t getSize() const;

	private:
		uint32_t version_ = 0;
		std::array<uint32_t, 8> sha256_ = { 0 };
		int64_t size_ = 0;
	};
}
