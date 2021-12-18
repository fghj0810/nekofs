#pragma once
#include "noncopyable.h"
#include "nonmovable.h"

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

namespace nekofs {
	class NativeFileSystem;

	class sha256sum final : private noncopyable, private nonmovable
	{
	public:
		sha256sum();
		void update(const void* data, size_t count);
		void final(const void* data = nullptr, size_t count = 0);
		void readHash(std::array<uint8_t, 32>& result) const;
		const std::array<uint32_t, 8>& readHash() const;
		std::string readHashHexString() const;

	private:
		std::array<uint32_t, 16> message_;
		std::array<uint32_t, 8> h_;
		size_t messageLength_ = 0;
		uint64_t bitLength_ = 0;
	};
}
