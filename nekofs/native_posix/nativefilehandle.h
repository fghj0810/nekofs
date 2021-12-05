#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <cstdint>
#include <memory>
#include <map>

namespace nekofs {
	class NativeFile;

	class NativeFileHandle final: public FileHandle, private noncopyable, private nonmovable
	{
	public:
		NativeFileHandle(std::shared_ptr<NativeFile> fPtr);

	public:
		void print() const override;

	private:
		std::shared_ptr<NativeFile> file_;
	};
}
