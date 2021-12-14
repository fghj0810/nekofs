﻿#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"
#include "../common/rapidjson.h"

#include <cstdint>
#include <memory>
#include <map>
#include <optional>

namespace nekofs {
	class LayerFilesMeta final
	{
	public:
		class FileMeta;

	public:
		static bool load(LayerFilesMeta& lfmeta, std::shared_ptr<IStream> is);
		static bool load(LayerFilesMeta& lfmeta, const JSONValue* jsondoc);
		static bool save(const LayerFilesMeta& lfmeta, std::shared_ptr<OStream> os);
		static bool save(const LayerFilesMeta& lfmeta, JSONValue* jsondoc, JSONDocument::AllocatorType& allocator);
		void setFileMeta(const std::string& filename, const FileMeta& meta);
		std::optional<FileMeta> getFileMeta(const std::string& filename) const;
		void setDeleteVersion(const std::string& filename, const uint32_t& version);
		std::optional<uint32_t> getDeleteVersion(const std::string& filename) const;

	private:
		static bool load_internal(LayerFilesMeta& lfmeta, const JSONValue* jsondoc);

	private:
		std::map<std::string, FileMeta> files_;
		std::map<std::string, uint32_t> deletes_;
	};


	class LayerFilesMeta::FileMeta final
	{
	public:
		void setVersion(const uint32_t& version);
		uint32_t getVersion() const;
		void setSHA256(const uint32_t sha256[8]);
		const uint32_t* getSHA256() const;
		void setSize(const int64_t& size);
		int64_t getSize() const;

	private:
		uint32_t version_ = 0;
		uint32_t sha256_[8] = { 0 };
		int64_t size_ = 0;
	};
}