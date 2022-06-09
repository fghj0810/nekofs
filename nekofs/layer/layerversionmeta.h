#pragma once

#include "../common/typedef.h"


#include "../common/rapidjson.h"

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <optional>

namespace nekofs {
	class LayerVersionMeta final
	{
	public:
		class SubResourceMeta;
	public:
		static std::optional<LayerVersionMeta> load(std::shared_ptr<IStream> is);
		static std::optional<LayerVersionMeta> load(const JSONValue* jsondoc);
		bool save(std::shared_ptr<OStream> os) const;
		bool save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator) const;
		void setName(const std::string& name);
		const std::string& getName() const;
		void setFromVersion(const uint32_t& fromVersion);
		uint32_t getFromVersion() const;
		void setVersion(const uint32_t& version);
		uint32_t getVersion() const;
		void addVersionServer(const std::string& server);
		void cleanVersionServers();
		const std::vector<std::string>& getVersionServers() const;
		void addDownloadServer(const std::string& server);
		void cleanDownloadServers();
		const std::vector<std::string>& getDownloadServers() const;
		void addRelocateServer(const std::string& server);
		void cleanRelocateServers();
		const std::vector<std::string>& getRelocateServers() const;
		void addSubResource(const std::string& name, const LayerVersionMeta::SubResourceMeta& meta);
		void removeSubResource(const std::string& name);
		void cleanSubResources();
		std::optional<LayerVersionMeta::SubResourceMeta> getSubResource(const std::string& name) const;
		const std::map<std::string, LayerVersionMeta::SubResourceMeta>& getSubResources() const;

	private:
		std::string name_;
		uint32_t fromVersion_ = 0;
		uint32_t version_ = 0;
		std::vector<std::string> versionServers_;
		std::vector<std::string> downloadServers_;
		std::vector<std::string> relocate_;
		std::map<std::string, LayerVersionMeta::SubResourceMeta> subResources_;
	};


	class LayerVersionMeta::SubResourceMeta final
	{
	public:
		static std::optional<LayerVersionMeta::SubResourceMeta> load(const JSONValue* jsondoc);
		bool save(JSONValue* jsondoc, JSONDocument::AllocatorType& allocator) const;
		void setRequire(const bool& require);
		bool getRequire() const;
		void addVersionServer(const std::string& server);
		void cleanVersionServers();
		const std::vector<std::string>& getVersionServers() const;

	private:
		bool require_ = false;
		std::vector<std::string> versionServers_;
	};
}
