#pragma once

#include "../common/typedef.h"
#include <cstdint>
#include <memory>

namespace nekofs {
	class AssetManagerFile;

	class AssetManagerIStream final : public IStream, public std::enable_shared_from_this<AssetManagerIStream>
	{
	private:
		AssetManagerIStream(const AssetManagerIStream&) = delete;
		AssetManagerIStream(const AssetManagerIStream&&) = delete;
		AssetManagerIStream& operator=(const AssetManagerIStream&) = delete;
		AssetManagerIStream& operator=(const AssetManagerIStream&&) = delete;

	public:
		AssetManagerIStream(std::shared_ptr<AssetManagerFile> file, int64_t fileSize);

	public:
		int32_t read(void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;
		std::shared_ptr<IStream> createNew() override;

	private:
		std::shared_ptr<AssetManagerFile> file_;
		int64_t fileSize_ = 0;
		int64_t position_ = 0;
	};
}
