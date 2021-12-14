#include "nekofs.h"

#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef _WIN32
const char* test_arg = u8"{\
    \"nativepath\": \"D:/test\"\
}";
#else
const char* test_arg = u8"{\
    \"nativepath\": \"/mnt/d/test\"\
}";
#endif

extern "C" {
	void log111(int32_t level, const char* str)
	{
		switch (level)
		{
		case NEKOFS_LOGINFO:
			std::cout << "[INFO]  " << str << std::endl;
			break;
		case NEKOFS_LOGWARN:
			std::cout << "[WARN]  " << str << std::endl;
			break;
		case NEKOFS_LOGERR:
			std::cout << "[ERRO]  " << str << std::endl;
			break;
		default:
			break;
		}
	}
}

int main()
{
	nekofs_SetLogDelegate(log111);
	if (nekofs_tools_prepare(test_arg))
	{
		return -1;
	}
}
