﻿#pragma once

#include "../common/typedef.h"
#include "../common/lz4.h"

#include <cstdint>
#include <array>
#include <string>
#include <memory>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <any>
#include <optional>

namespace nekofs {
	class NekodataFileMeta final
	{
	public:
		void setSHA256(const std::array<uint32_t, 8>& sha256);
		const std::array<uint32_t, 8>& getSHA256() const;
		void setCompressedSize(int64_t compressedSize);
		int64_t getCompressedSize() const;
		void setOriginalSize(int64_t originalSize);
		int64_t getOriginalSize() const;
		void addBlock(int32_t blockSize);
		const std::vector<int32_t>& getBlocks() const;

	private:
		std::array<uint32_t, 8> sha256_ = {};
		int64_t compressedSize_ = 0;
		int64_t originalSize_ = 0;
		std::vector<int32_t> blocks_;
	};
	class NativeFileSystem;

	class NekodataNativeArchiver final : public std::enable_shared_from_this<NekodataNativeArchiver>
	{
		NekodataNativeArchiver(const NekodataNativeArchiver&) = delete;
		NekodataNativeArchiver(NekodataNativeArchiver&&) = delete;
		NekodataNativeArchiver& operator=(const NekodataNativeArchiver&) = delete;
		NekodataNativeArchiver& operator=(NekodataNativeArchiver&&) = delete;
	public:
		enum class FileCategory
		{
			File,
			RawStream,
			Archiver
		};
		class FileTask final
		{
			FileTask(const FileTask&) = delete;
			FileTask(FileTask&&) = delete;
			FileTask& operator=(const FileTask&) = delete;
			FileTask& operator=(FileTask&&) = delete;
		public:
			enum class Status {
				None,
				InProgress,
				Finish,
				Error
			};
		public:
			FileTask(const std::string& path, std::shared_ptr<IStream> is, int64_t index);
			void setStatus(Status status);
			Status getStatus();
			int64_t getIndex() const;
			std::tuple<int64_t, int64_t> getRange() const;
			const std::string& getPath() const;
			std::shared_ptr<IStream> getIStream() const;
			void setBuffer(std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> buffer);
			std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> getBuffer() const;
			void setCompressedSize(int32_t size);
			int32_t getCompressedSize() const;
			bool isFinalTask() const;

		private:
			Status status_ = Status::None;
			int64_t index_ = 0;
			std::string path_;
			std::shared_ptr<IStream> is_;
			std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> compressBuffer_;
			int32_t compressedSize_ = 0;
			std::mutex mtx_;
		};
	public:
		NekodataNativeArchiver() = default;
		~NekodataNativeArchiver();
		void addFile(const std::string& filepath, std::shared_ptr<FileSystem> srcfs, const std::string& srcfilepath);
		void addRawFile(const std::string& filepath, std::shared_ptr<IStream> is);
		std::shared_ptr<NekodataNativeArchiver> addArchive(const std::string& filepath);
		bool archive(std::shared_ptr<OStream> os);

	private:
		bool archiveHeader();
		bool archiveFiles();
		bool archiveFooter();

	private:
		void threadfunction();

	private:
		std::shared_ptr<OStream> os_;
		std::map<std::string, std::pair<FileCategory, std::any>> archiveFileList_;
		std::mutex mtx_archiveFileList_;
		std::map<std::string, NekodataFileMeta> files_;
		std::queue<std::shared_ptr<FileTask>> taskList_;
		std::mutex mtx_taskList_;
		std::condition_variable cond_getTask_;
		std::condition_variable cond_finishTask_;
	};
}