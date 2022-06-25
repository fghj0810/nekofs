#pragma once

#include "../common/typedef.h"
#include <cstdint>
#include <memory>
#include <map>

namespace nekofs {
	class NativeFile;

	class NativeFileHandle final: public FileHandle
	{
	private:
		NativeFileHandle(const NativeFileHandle&) = delete;
		NativeFileHandle(NativeFileHandle&&) = delete;
		NativeFileHandle& operator=(const NativeFileHandle&) = delete;
		NativeFileHandle& operator=(NativeFileHandle&&) = delete;

	public:
		NativeFileHandle(std::shared_ptr<NativeFile> fPtr);

	public:
		void print() const override;

	private:
		std::shared_ptr<NativeFile> file_;
	};
}
