#include "env.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif


namespace nekofs {
	env env::instance_;

	env::env()
	{
		nativefilesystem_ = std::make_shared<NativeFileSystem>();
	}

	env::~env()
	{
		nativefilesystem_.reset();
	}

	env& env::getInstance()
	{
		return env::instance_;
	}

	void env::setLogDelegate(logdelegate* delegate)
	{
		this->log_ = delegate;
	}

	logdelegate* env::getLogDelegate()
	{
		return this->log_;
	}

	int32_t env::genId()
	{
		std::lock_guard<std::mutex> lock(mutex_gid_);
		if (idqueue_.size() == 0)
		{
			for (size_t i = 0; i < 10; i++)
			{
				gid_++;
				idqueue_.push(gid_);
			}
		}
		int32_t id = idqueue_.front();
		idqueue_.pop();
		return id;
	}

	void env::ungenId(int32_t id)
	{
		std::lock_guard<std::mutex> lock(mutex_gid_);
		idqueue_.push(id);
	}

	std::shared_ptr<NativeFileSystem> env::getNativeFileSystem() const
	{
		return nativefilesystem_;
	}
}
