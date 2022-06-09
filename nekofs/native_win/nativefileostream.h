#pragma once

#include "../common/typedef.h"
#include <Windows.h>
#include <cstdint>
#include <memory>

namespace nekofs {
	class NativeFile;

	class NativeOStream final : public OStream, public std::enable_shared_from_this<NativeOStream>
	{
	private:
		NativeOStream(const NativeOStream&) = delete;
		NativeOStream(const NativeOStream&&) = delete;
		NativeOStream& operator=(const NativeOStream&) = delete;
		NativeOStream& operator=(const NativeOStream&&) = delete;

	public:
		NativeOStream(std::shared_ptr<NativeFile> file, HANDLE fd);

	public:
		int32_t read(void* buf, int32_t size) override;
		int32_t write(const void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;

	private:
		std::shared_ptr<NativeFile> file_;
		HANDLE fd_ = INVALID_HANDLE_VALUE;
	};
}
