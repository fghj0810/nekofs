#pragma once

#include "../common/typedef.h"
#include "../common/lz4.h"
#include "nekodatafilemeta.h"

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
#include <tuple>
#include <atomic>
#include <functional>

namespace nekofs {
	class NekodataOStream;
	class NekodataVolumeOStream;

	class NekodataNativeArchiver final : public std::enable_shared_from_this<NekodataNativeArchiver>
	{
		friend class NekodataOStream;
		NekodataNativeArchiver(const NekodataNativeArchiver&) = delete;
		NekodataNativeArchiver(NekodataNativeArchiver&&) = delete;
		NekodataNativeArchiver& operator=(const NekodataNativeArchiver&) = delete;
		NekodataNativeArchiver& operator=(NekodataNativeArchiver&&) = delete;
	public:
		enum class FileCategory
		{
			None,
			File,
			Buffer,
			RawNekodataStream,
			Archiver
		};
		struct ArchiveInfo_File final
		{
			std::shared_ptr<FileSystem> fs;
			std::string filepath;
			int64_t length = 0;
			int64_t compressIndex = 0;
		};
		struct ArchiveInfo_Buffer final
		{
			const void* buffer;
			int64_t length = 0;
		};
		struct ArchiveInfo_RawNekodataStream final
		{
			std::shared_ptr<IStream> is;
			NekodataFileMeta meta;
		};
		class FileBlockTask final
		{
			FileBlockTask(const FileBlockTask&) = delete;
			FileBlockTask(FileBlockTask&&) = delete;
			FileBlockTask& operator=(const FileBlockTask&) = delete;
			FileBlockTask& operator=(FileBlockTask&&) = delete;
		public:
			enum class Status {
				None,
				Finish,
				Error
			};
		public:
			FileBlockTask(const std::string& path, std::shared_ptr<IStream> is, int64_t index);
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
		NekodataNativeArchiver(const std::string& archiveFilename, int64_t volumeSize = nekofs_kNekodata_DefalutVolumeSize, bool streamMode = false);
		void addFile(const std::string& filepath, std::shared_ptr<FileSystem> srcfs, const std::string& srcfilepath);
		void addBuffer(const std::string& filepath, const void* buffer, int64_t length);
		void addRawFile(const std::string& filepath, std::shared_ptr<IStream> is, const NekodataFileMeta& meta);
		std::shared_ptr<NekodataNativeArchiver> addArchive(const std::string& filepath);
		bool archive(std::function<void()> completeOneCallback = nullptr);

	private:
		bool archive(std::shared_ptr<OStream> os, const std::string& progressInfo, std::function<void()> completeOneCallback = nullptr);
		bool archiveFileHeader(std::shared_ptr<OStream> os);
		bool archiveFiles();
		bool archiveCentralDirectory();
		bool archiveFileFooters();
		std::shared_ptr<NekodataVolumeOStream> getVolumeOStreamByDataPos(int64_t pos);


	private:
		void threadfunction();

	private:
		bool finish = false;
		std::string archiveFilename_;
		int64_t volumeSize_ = nekofs_kNekodata_MaxVolumeSize;
		bool isStreamMode_ = false;
		std::shared_ptr<OStream> rawOS_;
		std::string progressInfo_;
		std::function<void ()> completeOneCallback_ = nullptr;
		std::shared_ptr<NekodataOStream> os_;
		std::vector<std::tuple<std::string, std::shared_ptr<NekodataVolumeOStream>>> volumeOS_;
		std::map<std::string, std::pair<FileCategory, std::any>> archiveFileList_;
		std::mutex mtx_archiveFileList_;
		std::map<std::string, NekodataFileMeta> files_;
		std::queue<std::shared_ptr<FileBlockTask>> taskList_;
		std::mutex mtx_taskList_;
		std::condition_variable cond_getTask_;
		std::condition_variable cond_finishTask_;
	};
}
