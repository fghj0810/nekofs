﻿#pragma once

#include "../common/typedef.h"
#include "../common/noncopyable.h"
#include "../common/nonmovable.h"

#include <Windows.h>
#include <cstdint>
#include <memory>
#include <map>
#include <mutex>

namespace nekofs {
	class NativeIStream;
	class NativeOStream;
	class NativeFileBlock;

	class NativeFile final: private noncopyable, private nonmovable, public std::enable_shared_from_this<NativeFile>
	{
	public:
		NativeFile(const std::string& filepath);
		const std::string& getFilePath() const;
		std::shared_ptr<NativeIStream> openIStream();
		std::shared_ptr<NativeOStream> openOStream();
		void createParentDirectory();
		std::shared_ptr<NativeFileBlock> openBlockInternal(int64_t offset);

	private:
		static void weakWriteDeleteCallback(std::weak_ptr<NativeFile> file, NativeOStream* ostream);
		static void weakReadDeleteCallback(std::weak_ptr<NativeFile> file, NativeIStream* istream);
		static void weakBlockDeleteCallback(std::weak_ptr<NativeFile> file, NativeFileBlock* block);
		void closeBlockInternal(int64_t offset);
		void openReadFdInternal();
		void closeReadFdInternal();
		void openWriteFdInternal();
		void closeWriteFdInternal();

	private:
		std::string filepath_;
		HANDLE readFd_ = INVALID_HANDLE_VALUE;
		int64_t readFileSize_ = 0;
		HANDLE readMapFd_ = NULL;
		HANDLE writeFd_ = INVALID_HANDLE_VALUE;
		int32_t readStreamCount_ = 0;
		std::vector<NativeFileBlock*> blockPtrs_;
		std::vector<std::weak_ptr<NativeFileBlock>> blocks_;
		std::recursive_mutex mtx_;
	};
}
