#pragma once

#include "../common/typedef.h"
#include <Windows.h>
#include <cstdint>
#include <memory>

namespace nekofs {
	class NativeFile;

	class NativeFileBlock final
	{
	private:
		NativeFileBlock(const NativeFileBlock&) = delete;
		NativeFileBlock(NativeFileBlock&&) = delete;
		NativeFileBlock& operator=(const NativeFileBlock&) = delete;
		NativeFileBlock& operator=(NativeFileBlock&&) = delete;

	public:
		NativeFileBlock(std::shared_ptr<NativeFile> file, HANDLE readMapFd, int64_t offset, int32_t size);
		int32_t read(int64_t pos, void* buffer, int32_t count);
		int64_t getOffset() const;
		int64_t getEndOffset() const;
		void mmap();
		void munmap();

	private:
		std::shared_ptr<NativeFile> file_;
		HANDLE readMapFd_ = NULL;
		LPCVOID lpBaseAddress_ = NULL;
		int64_t offset_;
		int32_t size_;
	};
}
