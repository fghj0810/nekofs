#pragma once
#include "typedef.h"
#include "noncopyable.h"
#include "nonmovable.h"

#include <atomic>
#include <queue>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace nekofs {
	class NativeFileSystem;

	class env final : private noncopyable, private nonmovable
	{
	private:
		static env instance_;
		env();
		~env();

	public:
		static env& getInstance();
		void setLogDelegate(logdelegate* delegate);
		logdelegate* getLogDelegate();
		int32_t genId();
		void ungenId(int32_t id);
		std::shared_ptr<NativeFileSystem> getNativeFileSystem() const;
	private:
		std::atomic<logdelegate*> log_;
		int32_t gid_ = 0;
		std::queue<int32_t> idqueue_;
		std::mutex mutex_gid_;
		std::shared_ptr<NativeFileSystem> nativefilesystem_;
	};
}
