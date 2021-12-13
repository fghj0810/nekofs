#include "typedef.h"
#include "env.h"
#include "utils.h"

namespace nekofs {
	void logprint(const LogType& level, const fschar* message)
	{
		auto cb = env::getInstance().getLogDelegate();
		if (cb != nullptr)
		{
			switch (level)
			{
			case nekofs::LogType::Info:
				cb(NEKOFS_LOGINFO, const_cast<fschar*>(message));
				break;
			case nekofs::LogType::Warning:
				cb(NEKOFS_LOGWARN, const_cast<fschar*>(message));
				break;
			case nekofs::LogType::Error:
				cb(NEKOFS_LOGERR, const_cast<fschar*>(message));
				break;
			default:
				break;
			}
		}
	}
	void logprint(const LogType& level, const fsstring& message)
	{
		logprint(level, message.c_str());
	}

	SeekOrigin Int2SeekOrigin(const NekoFSOrigin& value)
	{
		switch (value)
		{
		case NEKOFS_BEGIN:
			return SeekOrigin::Begin;
		case NEKOFS_CURRENT:
			return SeekOrigin::Current;
		case NEKOFS_END:
			return SeekOrigin::End;
		}
		return SeekOrigin::Unknown;
	}

	int32_t istream_read(std::shared_ptr<IStream>& is, void* buf, const int32_t& size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		int32_t size_tmp = size;
		uint8_t* buffer = static_cast<uint8_t*>(buf);
		int32_t actulRead = 0;
		do
		{
			actulRead = is->read(buffer, size_tmp);
			if (actulRead > 0)
			{
				size_tmp -= actulRead;
				buffer += actulRead;
			}
		} while (actulRead > 0 && size_tmp > 0);
		if (actulRead < 0 && size == size_tmp)
		{
			return actulRead;
		}
		return size - size_tmp;
	}

	int32_t ostream_write(std::shared_ptr<OStream>& os, const void* buf, const int32_t& size)
	{
		if (size < 0)
		{
			return -1;
		}
		if (size == 0)
		{
			return 0;
		}
		int32_t size_tmp = size;
		const uint8_t* buffer = static_cast<const uint8_t*>(buf);
		int32_t actulWrite = 0;
		do
		{
			actulWrite = os->write(buffer, size_tmp);
			if (actulWrite > 0)
			{
				size_tmp -= actulWrite;
				buffer += actulWrite;
			}
		} while (actulWrite > 0 && size_tmp > 0);
		if (actulWrite < 0 && size == size_tmp)
		{
			return actulWrite;
		}
		return size - size_tmp;
	}

#ifdef _WIN32
	static inline uint8_t hextob(fschar ch)
	{
		if (ch >= L'0' && ch <= L'9') return ch - L'0';
		if (ch >= L'A' && ch <= L'F') return ch - L'A' + 10;
		if (ch >= L'a' && ch <= L'f') return ch - L'a' + 10;
		return 0;
	}
#else
	static inline uint8_t hextob(fschar ch)
	{
		if (ch >= '0' && ch <= '9') return ch - '0';
		if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
		if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
		return 0;
	}
#endif
	void str_to_sha256(const fschar* in, uint32_t out[8])
	{
		for (size_t i = 0; i < 8; i++, in += 8)
		{
			out[i] = (out[i] << 4) | hextob(in[0]);
			out[i] = (out[i] << 4) | hextob(in[1]);
			out[i] = (out[i] << 4) | hextob(in[2]);
			out[i] = (out[i] << 4) | hextob(in[3]);
			out[i] = (out[i] << 4) | hextob(in[4]);
			out[i] = (out[i] << 4) | hextob(in[5]);
			out[i] = (out[i] << 4) | hextob(in[6]);
			out[i] = (out[i] << 4) | hextob(in[7]);
		}
	}
	fsstring sha256_to_str(const uint32_t out[8])
	{
#ifdef _WIN32
		const fschar kh[16] = { L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',L'8',L'9',L'a',L'b',L'c',L'd',L'e',L'f' };
#else
		const fschar kh[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
#endif
		fsstring str(64, 0);
		for (size_t i = 0; i < 8; i++)
		{
			str[0 + i * 8] = kh[(out[i] & 0xF0000000) >> 28];
			str[1 + i * 8] = kh[(out[i] & 0x0F000000) >> 24];
			str[2 + i * 8] = kh[(out[i] & 0x00F00000) >> 20];
			str[3 + i * 8] = kh[(out[i] & 0x000F0000) >> 16];
			str[4 + i * 8] = kh[(out[i] & 0x0000F000) >> 12];
			str[5 + i * 8] = kh[(out[i] & 0x00000F00) >> 8];
			str[6 + i * 8] = kh[(out[i] & 0x000000F0) >> 4];
			str[7 + i * 8] = kh[(out[i] & 0x0000000F) >> 0];
		}
		return str;
	}
}

