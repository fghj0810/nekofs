#include "env.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

#include <functional>

#ifdef ANDROID
#include "../assetmanager/assetmanagerfilesystem.h"

#include <jni.h>
#include <android/asset_manager_jni.h>

static AAssetManager* _assetManagerPtr = nullptr;

JNIEXPORT void JNICALL Java_com_pixelneko_FileSystem_initAssetManager2(JNIEnv *env, jclass cls, jobject assetManager)
{
	_assetManagerPtr = ::AAssetManager_fromJava(env, assetManager);
}
#endif

namespace nekofs {
	env env::instance_;

	env::env()
	{
		nativefilesystem_ = std::make_shared<NativeFileSystem>();
#ifdef ANDROID
		assetmanagerfilesystem_ = std::make_shared<AssetManagerFileSystem>();
#endif
	}

	env::~env()
	{
		nativefilesystem_.reset();
#ifdef ANDROID
		assetmanagerfilesystem_.reset();
#endif
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
	std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> env::newBufferBlockSize()
	{
		std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>* rawPtr = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex_Buffer_BlockSize_);
			if (!buffer_BlockSize_.empty())
			{
				rawPtr = buffer_BlockSize_.front();
				buffer_BlockSize_.pop();
			}
		}
		if (rawPtr == nullptr)
		{
			rawPtr = new std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>();
		}
		return std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>>(rawPtr, std::bind(&env::deleteBufferBlockSize, this, std::placeholders::_1));
	}
	std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> env::newBufferCompressSize()
	{
		std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>* rawPtr = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex_Buffer_CompressSize_);
			if (!buffer_CompressSize_.empty())
			{
				rawPtr = buffer_CompressSize_.front();
				buffer_CompressSize_.pop();
			}
		}
		if (rawPtr == nullptr)
		{
			rawPtr = new std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>();
		}
		return std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>>(rawPtr, std::bind(&env::deleteBufferCompressSize, this, std::placeholders::_1));
	}
	std::shared_ptr<std::array<uint8_t, 4 * 1024 * 1024>> env::newBuffer4M()
	{
		std::array<uint8_t, 4 * 1024 * 1024>* rawPtr = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex_Buffer_4M_);
			if (!buffer_4M_.empty())
			{
				rawPtr = buffer_4M_.front();
				buffer_4M_.pop();
			}
		}
		if (rawPtr == nullptr)
		{
			rawPtr = new std::array<uint8_t, 4 * 1024 * 1024>();
		}
		return std::shared_ptr<std::array<uint8_t, 4 * 1024 * 1024>>(rawPtr, std::bind(&env::deleteBuffer4M, this, std::placeholders::_1));
	}
	void env::deleteBufferBlockSize(std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>* buffer)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_Buffer_BlockSize_);
			if (buffer_BlockSize_.size() < 32)
			{
				buffer_BlockSize_.push(buffer);
				return;
			}
		}
		delete buffer;
	}
	void env::deleteBufferCompressSize(std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>* buffer)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_Buffer_CompressSize_);
			if (buffer_CompressSize_.size() < 32)
			{
				buffer_CompressSize_.push(buffer);
				return;
			}
		}
		delete buffer;
	}
	void env::deleteBuffer4M(std::array<uint8_t, 4 * 1024 * 1024>* buffer)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_Buffer_4M_);
			if (buffer_4M_.size() < 2)
			{
				buffer_4M_.push(buffer);
				return;
			}
		}
		delete buffer;
	}


#ifdef ANDROID
	AAssetManager* env::getAssetManagerPtr()
	{
		return _assetManagerPtr;
	}
	std::shared_ptr<AssetManagerFileSystem> env::getAssetManagerFileSystem() const
	{
		return assetmanagerfilesystem_;
	}
#endif
}
