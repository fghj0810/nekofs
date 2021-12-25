#pragma once
#include "typedef.h"
#include "lz4.h"

#include <atomic>
#include <queue>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <array>

namespace nekofs {
	class NativeFileSystem;

	class env final
	{
		env(const env&) = delete;
		env(env&&) = delete;
		env& operator=(const env&) = delete;
		env& operator=(env&&) = delete;
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
		std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> newBufferBlockSize();
		std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> newBufferCompressSize();
		std::shared_ptr<std::array<uint8_t, 4 * 1024 * 1024>> newBuffer4M();
	private:
		void deleteBufferBlockSize(std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>* buffer);
		void deleteBufferCompressSize(std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>* buffer);
		void deleteBuffer4M(std::array<uint8_t, 4 * 1024 * 1024>* buffer);

	private:
		std::atomic<logdelegate*> log_;
		int32_t gid_ = 0;
		std::queue<int32_t> idqueue_;
		std::mutex mutex_gid_;
		std::shared_ptr<NativeFileSystem> nativefilesystem_;
		std::mutex mutex_Buffer_BlockSize_;
		std::queue<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>*> buffer_BlockSize_;
		std::mutex mutex_Buffer_CompressSize_;
		std::queue<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>*> buffer_CompressSize_;
		std::mutex mutex_Buffer_4M_;
		std::queue<std::array<uint8_t, 4 * 1024 * 1024>*> buffer_4M_;
	};
}
