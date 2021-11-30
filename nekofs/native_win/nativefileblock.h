﻿#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <Windows.h>
#include <cstdint>
#include <memory>

namespace nekofs {
	class NativeFile;

	class NativeFileBlock final: private noncopyable, private nonmovable
	{
	public:
		NativeFileBlock(std::shared_ptr<NativeFile> file, int64_t offset, int32_t size);
		~NativeFileBlock();
		int32_t read(int64_t pos, void* buffer, int32_t count);
		int64_t getOffset() const;
		int64_t getEndOffset() const;

	private:
		std::shared_ptr<NativeFile> file_;
		LPCVOID lpBaseAddress_;
		int64_t offset_;
		int32_t size_;
	};
}
