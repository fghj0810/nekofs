#pragma once

#include "../common/typedef.h"
#include <cstdint>
#include <memory>
#include <map>
#include <mutex>

namespace nekofs {
	class NativeIStream;
	class NativeOStream;
	class NativeFileBlock;

	class NativeFile final: public std::enable_shared_from_this<NativeFile>
	{
	private:
		NativeFile(const NativeFile&) = delete;
		NativeFile(NativeFile&&) = delete;
		NativeFile& operator=(const NativeFile&) = delete;
		NativeFile& operator=(NativeFile&&) = delete;

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
		int readFd_ = -1;
		int64_t readFileSize_ = 0;
		int writeFd_ = -1;
		int32_t readStreamCount_ = 0;
		std::vector<NativeFileBlock*> blockPtrs_;
		std::vector<std::weak_ptr<NativeFileBlock>> blocks_;
		std::recursive_mutex mtx_;
	};
}
