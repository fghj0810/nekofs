#include "nativefile.h"
#include "nativefileistream.h"
#include "nativefileostream.h"
#include "nativefilesystem.h"
#include "nativefileblock.h"
#include "../common/utils.h"
#include "../common/env.h"

#include <sstream>
#include <functional>
#include <filesystem>

namespace nekofs {
	NativeFile::NativeFile(const std::string& filepath)
	{
		filepath_ = filepath;
	}
	const std::string& NativeFile::getFilePath() const
	{
		return filepath_;
	}
	std::shared_ptr<NativeIStream> NativeFile::openIStream()
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (INVALID_HANDLE_VALUE != writeFd_)
		{
			std::stringstream ss;
			ss << u8"NativeFile::openIStream file in use! filepath = ";
			ss << filepath_;
			logerr(ss.str());
			return nullptr;
		}
		openReadFdInternal();
		if (INVALID_HANDLE_VALUE == readFd_)
		{
			return nullptr;
		}
		readStreamCount_++;
		std::shared_ptr<NativeIStream> isPtr;
		isPtr.reset(new NativeIStream(shared_from_this(), readFileSize_), std::bind(&NativeFile::weakReadDeleteCallback, std::weak_ptr<NativeFile>(shared_from_this()), std::placeholders::_1));
		return isPtr;
	}
	std::shared_ptr<NativeOStream> NativeFile::openOStream()
	{
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		if (readStreamCount_ > 0 || INVALID_HANDLE_VALUE != writeFd_)
		{
			std::stringstream ss;
			ss << u8"NativeFile::openOStream file in use! filepath = ";
			ss << filepath_;
			logerr(ss.str());
			return nullptr;
		}
		openWriteFdInternal();
		if (INVALID_HANDLE_VALUE == writeFd_)
		{
			return nullptr;
		}
		std::shared_ptr<NativeOStream> sp;
		sp.reset(new NativeOStream(shared_from_this(), writeFd_), std::bind(&NativeFile::weakWriteDeleteCallback, std::weak_ptr<NativeFile>(shared_from_this()), std::placeholders::_1));
		return sp;
	}
	void NativeFile::createParentDirectory()
	{
		std::filesystem::path p(filepath_);
		if (p.parent_path() == p.root_path())
		{
			return;
		}
		env::getInstance().getNativeFileSystem()->createDirectories(filepath_.substr(0, filepath_.rfind(nekofs_PathSeparator)));
	}
	std::shared_ptr<NativeFileBlock> NativeFile::openBlockInternal(int64_t offset)
	{
		size_t index = offset >> nekofs_MapBlockSizeBitOffset;
		std::shared_ptr<NativeFileBlock> fPtr;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx_);
			fPtr = blocks_[index].lock();
			if (!fPtr)
			{
				NativeFileBlock* rawPtr = blockPtrs_[index];
				if (rawPtr == nullptr)
				{
					const int64_t offset = index << nekofs_MapBlockSizeBitOffset;
					const int32_t size = readFileSize_ - offset > nekofs_MapBlockSize ? nekofs_MapBlockSize : static_cast<int32_t>(readFileSize_ - offset);
					rawPtr = new NativeFileBlock(shared_from_this(), readMapFd_, offset, size);
					rawPtr->mmap();
					blockPtrs_[index] = rawPtr;
				}
				fPtr.reset(rawPtr, std::bind(&NativeFile::weakBlockDeleteCallback, std::weak_ptr<NativeFile>(shared_from_this()), std::placeholders::_1));
				blocks_[index] = fPtr;
			}
		}
		return fPtr;
	}


	void NativeFile::weakWriteDeleteCallback(std::weak_ptr<NativeFile> file, NativeOStream* ostream)
	{
		auto fp = file.lock();
		std::lock_guard<std::recursive_mutex> lock(fp->mtx_);
		fp->closeWriteFdInternal();
		delete ostream;
	}
	void NativeFile::weakReadDeleteCallback(std::weak_ptr<NativeFile> file, NativeIStream* istream)
	{
		auto fp = file.lock();
		std::lock_guard<std::recursive_mutex> lock(fp->mtx_);
		fp->readStreamCount_--;
		if (fp->readStreamCount_ == 0)
		{
			fp->closeReadFdInternal();
		}
		delete istream;
	}
	void NativeFile::weakBlockDeleteCallback(std::weak_ptr<NativeFile> file, NativeFileBlock* block)
	{
		auto fp = file.lock();
		fp->closeBlockInternal(block->getOffset());
	}
	void NativeFile::closeBlockInternal(int64_t offset)
	{
		size_t index = offset >> nekofs_MapBlockSizeBitOffset;
		std::lock_guard<std::recursive_mutex> lock(mtx_);
		auto sp = blocks_[index].lock();
		if (!sp)
		{
			blockPtrs_[index]->munmap();
			delete blockPtrs_[index];
			blockPtrs_[index] = nullptr;
		}
	}
	void NativeFile::openReadFdInternal()
	{
		if (INVALID_HANDLE_VALUE == readFd_)
		{
			readFd_ = CreateFile(u8_to_u16(filepath_).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_RANDOM_ACCESS, NULL);
			if (INVALID_HANDLE_VALUE == readFd_)
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeFile::openReadFdInternal CreateFile error! filepath = ";
				ss << filepath_;
				ss << u8", err = ";
				ss << errmsg;
				logerr(ss.str());
			}
			else
			{
				LARGE_INTEGER size;
				if (FALSE == GetFileSizeEx(readFd_, &size))
				{
					{
						auto errmsg = getSysErrMsg();
						std::stringstream ss;
						ss << u8"NativeFile::openReadFdInternal GetFileSizeEx error! filepath = ";
						ss << filepath_;
						ss << u8", err = ";
						ss << errmsg;
						logerr(ss.str());
					}
					if (FALSE == CloseHandle(readFd_))
					{
						auto errmsg = getSysErrMsg();
						std::stringstream ss;
						ss << u8"NativeFile::openReadFdInternal CloseHandle1 error! filepath = ";
						ss << filepath_;
						ss << u8", err = ";
						ss << errmsg;
						logerr(ss.str());
					}
					readFd_ = INVALID_HANDLE_VALUE;
				}
				else
				{
					readFileSize_ = size.QuadPart;
					const size_t vector_size = (readFileSize_ >> nekofs_MapBlockSizeBitOffset) + 1;
					blockPtrs_.resize(vector_size);
					blocks_.resize(vector_size);
					if (readFileSize_ > 0)
					{
						readMapFd_ = CreateFileMapping(readFd_, NULL, PAGE_READONLY, 0, 0, NULL);
						if (NULL == readFd_)
						{
							{
								auto errmsg = getSysErrMsg();
								std::stringstream ss;
								ss << u8"NativeFile::openReadFdInternal CreateFileMapping error! filepath = ";
								ss << filepath_;
								ss << u8", err = ";
								ss << errmsg;
								logerr(ss.str());
							}
							if (FALSE == CloseHandle(readFd_))
							{
								auto errmsg = getSysErrMsg();
								std::stringstream ss;
								ss << u8"NativeFile::openReadFdInternal CloseHandle2 error! filepath = ";
								ss << filepath_;
								ss << u8", err = ";
								ss << errmsg;
								logerr(ss.str());
							}
							readFd_ = INVALID_HANDLE_VALUE;
							readFileSize_ = 0;
						}
					}
				}
			}
		}
	}
	void NativeFile::closeReadFdInternal()
	{
		if (NULL != readMapFd_)
		{
			if (FALSE == CloseHandle(readMapFd_))
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeFile::closeReadFdInternal CloseHandle readMap error! filepath = ";
				ss << filepath_;
				ss << u8", err = ";
				ss << errmsg;
				logerr(ss.str());
			}
			readMapFd_ = NULL;
		}
		if (INVALID_HANDLE_VALUE != readFd_)
		{
			if (FALSE == CloseHandle(readFd_))
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeFile::closeReadFdInternal CloseHandle readFd error! filepath = ";
				ss << filepath_;
				ss << u8", err = ";
				ss << errmsg;
				logerr(ss.str());
			}
			readFd_ = INVALID_HANDLE_VALUE;
		}
		readFileSize_ = 0;
	}
	void NativeFile::openWriteFdInternal()
	{
		if (INVALID_HANDLE_VALUE == writeFd_)
		{
			createParentDirectory();
			writeFd_ = CreateFile(u8_to_u16(filepath_).c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			if (INVALID_HANDLE_VALUE == writeFd_)
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeFile::openWriteFdInternal CreateFile error ! filepath = ";
				ss << filepath_;
				ss << u8", err = ";
				ss << errmsg;
				logerr(ss.str());
			}
		}
	}
	void NativeFile::closeWriteFdInternal()
	{
		if (INVALID_HANDLE_VALUE != writeFd_)
		{
			if (FALSE == CloseHandle(writeFd_))
			{
				auto errmsg = getSysErrMsg();
				std::stringstream ss;
				ss << u8"NativeFile::closeWriteFdInternal CloseHandle error ! filepath = ";
				ss << filepath_;
				ss << u8", err = ";
				ss << errmsg;
				logerr(ss.str());
			}
			writeFd_ = INVALID_HANDLE_VALUE;
		}
	}
}
