#pragma once

#include "../common/typedef.h"
#include "../common/utils.h"

#include <cstdint>
#include <string>
#include <memory>

namespace nekofs {
	inline bool nekodata_readUint64(std::shared_ptr<IStream> is, uint64_t& value)
	{
		value = 0;
		if (is)
		{
			int32_t num = 0;
			uint8_t buffer[8];
			uint8_t v = 0;
			if (istream_read(is, &v, 1) == 1)
			{
				uint8_t tmp = v;
				while ((tmp & 0x80) != 0)
				{
					tmp <<= 1;
					num++;
				}
				if (num <= 8)
				{
					if (num < 7)
					{
						value = ((1 << (7 - num)) - 1) & v;
					}
					if (istream_read(is, buffer, num) == num)
					{
						for (int32_t i = 0; i < num; i++)
						{
							value <<= 8; value |= buffer[i];
						}
						return true;
					}
				}
			}
		}
		return false;
	}
	inline bool nekodata_writeUint64(std::shared_ptr<OStream> os, const uint64_t& value)
	{
		const uint64_t n1 = 0x0000000000000080;
		const uint64_t n2 = 0x0000000000004000;
		const uint64_t n3 = 0x0000000000200000;
		const uint64_t n4 = 0x0000000010000000;
		const uint64_t n5 = 0x0000000800000000;
		const uint64_t n6 = 0x0000040000000000;
		const uint64_t n7 = 0x0002000000000000;
		const uint64_t n8 = 0x0100000000000000;
		uint8_t buffer[9];
		int32_t num = 0;
		if (value < n4) {
			if (value < n2) {
				if (value < n1) {
					// 1
					num = 1;
					buffer[0] = static_cast<uint8_t>(value);
				}
				else {
					// 2
					num = 2;
					buffer[0] = static_cast<uint8_t>((value >> 8) | 0x80);
				}
			}
			else {
				if (value < n3) {
					// 3
					num = 3;
					buffer[0] = static_cast<uint8_t>((value >> 16) | 0xC0);
				}
				else {
					// 4
					num = 4;
					buffer[0] = static_cast<uint8_t>((value >> 24) | 0xE0);
				}
			}
		}
		else {
			if (value < n6) {
				if (value < n5) {
					// 5
					num = 5;
					buffer[0] = static_cast<uint8_t>((value >> 32) | 0xF0);
				}
				else {
					// 6
					num = 6;
					buffer[0] = static_cast<uint8_t>((value >> 40) | 0xF8);
				}
			}
			else {
				if (value < n7) {
					// 7
					num = 7;
					buffer[0] = static_cast<uint8_t>((value >> 48) | 0xFC);
				}
				else {
					if (value < n8) {
						// 8
						num = 8;
						buffer[0] = static_cast<uint8_t>((value >> 56) | 0xFE);
					}
					else {
						// 9
						num = 9;
						buffer[0] = 0xFF;
					}
				}
			}
		}
		for (int32_t i = 1; i < num; i++)
		{
			buffer[i] = static_cast<uint8_t>(value >> ((num - i - 1) * 8));
		}
		return ostream_write(os, buffer, num) == num;
	}
	inline bool nekodata_readUint32(std::shared_ptr<IStream> is, uint32_t& value)
	{
		value = 0;
		if (is)
		{
			int32_t num = 0;
			uint8_t buffer[4];
			uint8_t v = 0;
			if (istream_read(is, &v, 1) == 1)
			{
				uint8_t tmp = v;
				while ((tmp & 0x80) != 0)
				{
					tmp <<= 1;
					num++;
				}
				if (num <= 4)
				{
					value = ((1 << (7 - num)) - 1) & v;
					if (istream_read(is, buffer, num) == num)
					{
						for (int32_t i = 0; i < num; i++)
						{
							value <<= 8; value |= buffer[i];
						}
						return true;
					}
				}
			}
		}
		return false;
	}
	inline bool nekodata_writeUint32(std::shared_ptr<OStream> os, const uint32_t& value)
	{
		const uint32_t n1 = 0x0000000000000080;
		const uint32_t n2 = 0x0000000000004000;
		const uint32_t n3 = 0x0000000000200000;
		const uint32_t n4 = 0x0000000010000000;
		uint8_t buffer[5];
		int32_t num = 0;
		if (value < n2) {
			if (value < n1) {
				// 1
				num = 1;
				buffer[0] = static_cast<uint8_t>(value);
			}
			else {
				// 2
				num = 2;
				buffer[0] = static_cast<uint8_t>((value >> 8) | 0x80);
			}
		}
		else {
			if (value < n3) {
				// 3
				num = 3;
				buffer[0] = static_cast<uint8_t>((value >> 16) | 0xC0);
			}
			else {
				if (value < n4)
				{
					// 4
					num = 4;
					buffer[0] = static_cast<uint8_t>((value >> 24) | 0xE0);
				}
				else
				{
					num = 5;
					buffer[0] = 0xF0;
				}
			}
		}
		for (int32_t i = 1; i < num; i++)
		{
			buffer[i] = static_cast<uint8_t>(value >> ((num - i - 1) * 8));
		}
		return ostream_write(os, buffer, num) == num;
	}

	inline bool nekodata_readVolumeSize(std::shared_ptr<IStream> is, int64_t& volumeSize)
	{
		volumeSize = 0;
		uint8_t buffer[4];
		if (is && istream_read(is, buffer, 4) == 4)
		{
			volumeSize <<= 8; volumeSize |= buffer[0];
			volumeSize <<= 8; volumeSize |= buffer[1];
			volumeSize <<= 8; volumeSize |= buffer[2];
			volumeSize <<= 8; volumeSize |= buffer[3];
			volumeSize <<= 20;
			return true;
		}
		return false;
	}
	inline bool nekodata_writeVolumeSize(std::shared_ptr<OStream> os, const int64_t& volumeSize)
	{
		int64_t mask = (1ll << 20) - 1;
		if (os && volumeSize > 0 && (volumeSize & mask) == 0)
		{
			uint32_t value = static_cast<uint32_t>(volumeSize >> 20);
			uint8_t buffer[4];
			buffer[0] = static_cast<uint8_t>(value >> 24);
			buffer[1] = static_cast<uint8_t>(value >> 16);
			buffer[2] = static_cast<uint8_t>(value >> 8);
			buffer[3] = static_cast<uint8_t>(value >> 0);
			return ostream_write(os, buffer, 4) == 4;
		}
		return false;
	}
	inline bool nekodata_readVolumeNum(std::shared_ptr<IStream> is, uint32_t& volumeNum)
	{
		volumeNum = 0;
		uint8_t buffer[4];
		if (is && istream_read(is, buffer, 4) == 4)
		{
			volumeNum <<= 8; volumeNum |= buffer[0];
			volumeNum <<= 8; volumeNum |= buffer[1];
			volumeNum <<= 8; volumeNum |= buffer[2];
			volumeNum <<= 8; volumeNum |= buffer[3];
			return true;
		}
		return false;
	}
	inline bool nekodata_writeVolumeNum(std::shared_ptr<OStream> os, const uint32_t& volumeNum)
	{
		if (os)
		{
			uint8_t buffer[4];
			buffer[0] = static_cast<uint8_t>(volumeNum >> 24);
			buffer[1] = static_cast<uint8_t>(volumeNum >> 16);
			buffer[2] = static_cast<uint8_t>(volumeNum >> 8);
			buffer[3] = static_cast<uint8_t>(volumeNum >> 0);
			return ostream_write(os, buffer, 4) == 4;
		}
		return false;
	}
	inline bool nekodata_readCentralDirectoryPosition(std::shared_ptr<IStream> is, int64_t& position)
	{
		position = 0;
		uint8_t buffer[8];
		if (is && istream_read(is, buffer, 8) == 8)
		{
			position <<= 8; position |= buffer[0];
			position <<= 8; position |= buffer[1];
			position <<= 8; position |= buffer[2];
			position <<= 8; position |= buffer[3];
			position <<= 8; position |= buffer[4];
			position <<= 8; position |= buffer[5];
			position <<= 8; position |= buffer[6];
			position <<= 8; position |= buffer[7];
			return position >= 0;
		}
		return false;
	}
	inline bool nekodata_writeCentralDirectoryPosition(std::shared_ptr<OStream> os, const int64_t& position)
	{
		if (os && position >= 0)
		{
			uint8_t buffer[8];
			buffer[0] = static_cast<uint8_t>(position >> 56);
			buffer[1] = static_cast<uint8_t>(position >> 48);
			buffer[2] = static_cast<uint8_t>(position >> 40);
			buffer[3] = static_cast<uint8_t>(position >> 32);
			buffer[4] = static_cast<uint8_t>(position >> 24);
			buffer[5] = static_cast<uint8_t>(position >> 16);
			buffer[6] = static_cast<uint8_t>(position >> 8);
			buffer[7] = static_cast<uint8_t>(position >> 0);
			return ostream_write(os, buffer, 8) == 8;
		}
		return false;
	}
	inline bool nekodata_readStringLength(std::shared_ptr<IStream> is, int32_t& length)
	{
		uint32_t value;
		if (nekodata_readUint32(is, value) && static_cast<int32_t>(value) > 0)
		{
			length = static_cast<int32_t>(value);
			return true;
		}
		return false;
	}
	inline bool nekodata_writeStringLength(std::shared_ptr<OStream> os, const int32_t& length)
	{
		if (length > 0)
		{
			return nekodata_writeUint32(os, static_cast<uint32_t>(length));
		}
		return false;
	}
	inline bool nekodata_readString(std::shared_ptr<IStream> is, std::string& str)
	{
		str.clear();
		int32_t length;
		if (nekodata_readStringLength(is, length))
		{
			str.resize(length);
			if (istream_read(is, &str[0], length) == length)
			{
				return true;
			}
			str.clear();
		}
		return false;
	}
	inline bool nekodata_writeString(std::shared_ptr<OStream> os, const std::string& str)
	{
		int32_t length = static_cast<uint32_t>(str.length());
		if (static_cast<size_t>(length) == str.length() && nekodata_writeStringLength(os, length))
		{
			return ostream_write(os, str.data(), length) == length;
		}
		return false;
	}
	inline bool nekodata_readFileSize(std::shared_ptr<IStream> is, int64_t& fileSize)
	{
		uint64_t value;
		if (nekodata_readUint64(is, value))
		{
			fileSize = static_cast<int64_t>(value);
			return fileSize >= 0;
		}
		return false;
	}
	inline bool nekodata_writeFileSize(std::shared_ptr<OStream> os, const int64_t& fileSize)
	{
		if (fileSize >= 0)
		{
			return nekodata_writeUint64(os, static_cast<uint64_t>(fileSize));
		}
		return false;
	}
	inline bool nekodata_readBlockNum(std::shared_ptr<IStream> is, int64_t& blockNum)
	{
		uint64_t value;
		if (nekodata_readUint64(is, value))
		{
			blockNum = static_cast<int64_t>(value);
			return blockNum > 0;
		}
		return false;
	}
	inline bool nekodata_writeBlockNum(std::shared_ptr<OStream> os, const int64_t& blockNum)
	{
		if (blockNum > 0)
		{
			return nekodata_writeUint64(os, static_cast<uint64_t>(blockNum));
		}
		return false;
	}
	inline bool nekodata_readBlockSize(std::shared_ptr<IStream> is, int32_t& blockSize)
	{
		uint32_t value;
		if (nekodata_readUint32(is, value))
		{
			blockSize = static_cast<int32_t>(value);
			return blockSize > 0;
		}
		return false;
	}
	inline bool nekodata_writeBlockSize(std::shared_ptr<OStream> os, const int32_t& blockSize)
	{
		if (blockSize > 0)
		{
			return nekodata_writeUint32(os, static_cast<uint32_t>(blockSize));
		}
		return false;
	}
	inline bool nekodata_readPosition(std::shared_ptr<IStream> is, int64_t& position)
	{
		uint64_t value;
		if (nekodata_readUint64(is, value))
		{
			position = static_cast<int64_t>(value);
			return position >= 0;
		}
		return false;
	}
	inline bool nekodata_writePosition(std::shared_ptr<OStream> os, const int64_t& position)
	{
		if (position >= 0)
		{
			return nekodata_writeUint64(os, static_cast<uint64_t>(position));
		}
		return false;
	}
	inline bool nekodata_readSHA256(std::shared_ptr<IStream> is, std::array<uint32_t, 8>& hash)
	{
		bool success = is != nullptr;
		for (size_t i = 0; success && i < hash.size(); i++)
		{
			uint32_t value = 0;
			uint8_t buffer[4];
			success = istream_read(is, buffer, 4) == 4;
			if (success)
			{
				value <<= 8; value |= buffer[0];
				value <<= 8; value |= buffer[1];
				value <<= 8; value |= buffer[2];
				value <<= 8; value |= buffer[3];
				hash[i] = value;
			}
		}
		return success;
	}
	inline bool nekodata_writeSHA256(std::shared_ptr<OStream> os, const std::array<uint32_t, 8>& hash)
	{
		bool success = os != nullptr;
		uint8_t buffer[4];
		for (size_t i = 0; success && i < hash.size(); i++)
		{
			buffer[0] = static_cast<uint8_t>(hash[i] >> 24);
			buffer[1] = static_cast<uint8_t>(hash[i] >> 16);
			buffer[2] = static_cast<uint8_t>(hash[i] >> 8);
			buffer[3] = static_cast<uint8_t>(hash[i] >> 0);
			success = ostream_write(os, buffer, 4) == 4;
		}
		return success;
	}
}
