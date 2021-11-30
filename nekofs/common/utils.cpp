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

	SeekOrigin Int2SeekOrigin(const int32_t& value)
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
}

