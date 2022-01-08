﻿#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <cstdint>
#include <memory>

namespace nekofs {
	class NativeFile;

	class NativeOStream final : public OStream, private noncopyable, private nonmovable, public std::enable_shared_from_this<NativeOStream>
	{
	public:
		NativeOStream(std::shared_ptr<NativeFile> file, int fd);

	public:
		int32_t read(void* buf, int32_t size) override;
		int32_t write(const void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;

	private:
		std::shared_ptr<NativeFile> file_;
		int fd_ = -1;
	};
}
