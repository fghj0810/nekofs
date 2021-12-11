#include "sha256.h"

#include <algorithm>

namespace nekofs {
	constexpr uint32_t h[8] = {
		0x6a09e667,
		0xbb67ae85,
		0x3c6ef372,
		0xa54ff53a,
		0x510e527f,
		0x9b05688c,
		0x1f83d9ab,
		0x5be0cd19
	};
	constexpr uint32_t k[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	static inline uint32_t rightrotate(const uint32_t& value, int offset)
	{
		return (value >> offset) | (value << (32 - offset));
	}
	static inline uint32_t rightshift(const uint32_t& value, int offset)
	{
		return value >> offset;
	}
	static inline void sha256_calc(const uint32_t message[64], uint32_t h_result[8])
	{
		uint32_t w[64];
		std::copy(message, message + 16, w);
		for (size_t i = 16; i < 64; i++)
		{
			uint32_t s0 = rightrotate(w[i - 15], 7) ^ rightrotate(w[i - 15], 18) ^ rightshift(w[i - 15], 3);
			uint32_t s1 = rightrotate(w[i - 2], 17) ^ rightrotate(w[i - 2], 19) ^ rightshift(w[i - 2], 10);
			w[i] = w[i - 16] + s0 + w[i - 7] + s1;
		}
		uint32_t a = h_result[0];
		uint32_t b = h_result[1];
		uint32_t c = h_result[2];
		uint32_t d = h_result[3];
		uint32_t e = h_result[4];
		uint32_t f = h_result[5];
		uint32_t g = h_result[6];
		uint32_t h = h_result[7];
		for (size_t i = 0; i < 64; i++)
		{
			uint32_t s0 = rightrotate(a, 2) ^ rightrotate(a, 13) ^ rightrotate(a, 22);
			uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			uint32_t t2 = s0 + maj;
			uint32_t s1 = rightrotate(e, 6) ^ rightrotate(e, 11) ^ rightrotate(e, 25);
			uint32_t ch = (e & f) ^ ((~e) & g);
			uint32_t t1 = h + s1 + ch + k[i] + w[i];
			h = g;
			g = f;
			f = e;
			e = d + t1;
			d = c;
			c = b;
			b = a;
			a = t1 + t2;
		}
		h_result[0] += a;
		h_result[1] += b;
		h_result[2] += c;
		h_result[3] += d;
		h_result[4] += e;
		h_result[5] += f;
		h_result[6] += g;
		h_result[7] += h;
	}

	sha256sum::sha256sum()
	{
		std::copy(h, h + 8, h_);
	}
	void sha256sum::update(const void* data, size_t count)
	{
		const uint8_t* pBuf = static_cast<const uint8_t*>(data);
		size_t msg_index = messageLength_ >> 2;
		size_t head_count = (4 - (messageLength_ & 0x03)) & 0x03;
		if (head_count > 0)
		{
			if (head_count <= count)
			{
				for (size_t i = 0; i < head_count; i++) {
					message_[msg_index] = message_[msg_index] << 8 | pBuf[i];
				}
				pBuf += head_count;
				count -= head_count;
				bitLength_ += head_count << 3;
				messageLength_ += head_count;
				if (messageLength_ == 64) {
					sha256_calc(message_, h_);
					messageLength_ = 0;
				}
			}
			else
			{
				for (size_t i = 0; i < count; i++) {
					message_[msg_index] = message_[msg_index] << 8 | pBuf[i];
				}
				pBuf += count;
				bitLength_ += count << 3;
				messageLength_ += count;
				count = 0;
			}
		}
		if (count > 0)
		{
			msg_index = messageLength_ >> 2;
			size_t max_count = (count + messageLength_) >> 6;
			if (max_count > 0)
			{
				size_t length = max_count * 64 - messageLength_;
				count -= length;
				bitLength_ += length << 3;
				messageLength_ = 0;
				for (size_t i = 0; i < max_count; i++)
				{
					do
					{
						uint32_t a = static_cast<uint32_t>(pBuf[0]) << 24;
						uint32_t b = static_cast<uint32_t>(pBuf[1]) << 16;
						uint32_t c = static_cast<uint32_t>(pBuf[2]) << 8;
						uint32_t d = static_cast<uint32_t>(pBuf[3]) << 0;
						message_[msg_index] = a | b | c | d;
						msg_index++;
						pBuf += 4;
					} while (msg_index < 16);
					msg_index = 0;
					sha256_calc(message_, h_);
				}
			}

			if (count > 0)
			{
				max_count = count >> 2;
				msg_index = messageLength_ >> 2;
				if (max_count > 0)
				{
					size_t length = max_count << 2;
					count -= length;
					bitLength_ += length << 3;
					messageLength_ += length;
					for (size_t i = 0; i < max_count; i++) {
						uint32_t a = static_cast<uint32_t>(pBuf[0]) << 24;
						uint32_t b = static_cast<uint32_t>(pBuf[1]) << 16;
						uint32_t c = static_cast<uint32_t>(pBuf[2]) << 8;
						uint32_t d = static_cast<uint32_t>(pBuf[3]) << 0;
						message_[msg_index] = a | b | c | d;
						pBuf += 4;
						msg_index++;
					}
				}

				if (count > 0)
				{
					msg_index = messageLength_ >> 2;
					messageLength_ += count;
					bitLength_ += count << 3;
					for (size_t i = 0; i < count; i++) {
						message_[msg_index] = message_[msg_index] << 8 | pBuf[i];
					}
				}
			}
		}
	}
	void sha256sum::final(const void* data, size_t count)
	{
		update(data, count);
		{
			// padding
			if (messageLength_ < 56)
			{
				message_[messageLength_ >> 2] = message_[messageLength_ >> 2] << 8 | 0x080;
				messageLength_++;
			}
			else
			{
				message_[messageLength_ >> 2] = message_[messageLength_ >> 2] << 8 | 0x080;
				messageLength_++;
				while (messageLength_ < 64)
				{
					message_[messageLength_ >> 2] = message_[messageLength_ >> 2] << 8 | 0x00;
					messageLength_++;
				}
				sha256_calc(message_, h_);
				messageLength_ = 0;
			}
			while (messageLength_ < 56)
			{
				message_[messageLength_ >> 2] = message_[messageLength_ >> 2] << 8 | 0x00;
				messageLength_++;
			}
			message_[14] = bitLength_ >> 32;
			message_[15] = static_cast<uint32_t>(bitLength_);
			messageLength_ += 8;
			sha256_calc(message_, h_);
			messageLength_ = 0;
		}
	}
	void sha256sum::readHash(uint8_t result[32])
	{
		for (size_t i = 0; i < 8; i++)
		{
			result[(i << 2) + 0] = static_cast<uint8_t>(h_[i] >> 24);
			result[(i << 2) + 1] = static_cast<uint8_t>(h_[i] >> 16);
			result[(i << 2) + 2] = static_cast<uint8_t>(h_[i] >> 8);
			result[(i << 2) + 3] = static_cast<uint8_t>(h_[i] >> 0);
		}
	}
	void sha256sum::readHash(uint32_t result[8])
	{
		std::copy(h_, h_ + 8, result);
	}
}
