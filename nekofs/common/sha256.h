#pragma once
#include "noncopyable.h"
#include "nonmovable.h"

#include <cstdint>
#include <cstddef>

namespace nekofs {
	class NativeFileSystem;

	class sha256sum final : private noncopyable, private nonmovable
	{
	public:
		sha256sum();
		void update(const void* data, size_t count);
		void final(const void* data, size_t count, uint8_t result[32]);
		void readHash(uint8_t result[32]);

	private:
		uint32_t message_[16] = { 0 };
		uint32_t h_[8];
		size_t messageLength_ = 0;
		uint64_t bitLength_ = 0;
	};
}
