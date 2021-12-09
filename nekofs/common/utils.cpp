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
}

