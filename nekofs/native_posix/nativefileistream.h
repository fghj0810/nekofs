#pragma once

#include "../common/typedef.h"
#include <cstdint>
#include <memory>

namespace nekofs {
	class NativeFile;
	class NativeFileBlock;

	class NativeIStream final : public IStream, public std::enable_shared_from_this<NativeIStream>
	{
	private:
		NativeIStream(const NativeIStream&) = delete;
		NativeIStream(const NativeIStream&&) = delete;
		NativeIStream& operator=(const NativeIStream&) = delete;
		NativeIStream& operator=(const NativeIStream&&) = delete;

	public:
		NativeIStream(std::shared_ptr<NativeFile> file, int64_t fileSize);

	public:
		int32_t read(void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;
		std::shared_ptr<IStream> createNew() override;

	private:
		std::shared_ptr<NativeFileBlock> prepareBlock();

	private:
		std::shared_ptr<NativeFile> file_;
		std::shared_ptr<NativeFileBlock> block_;
		int64_t fileSize_ = 0;
		int64_t position_ = 0;
	};
}
