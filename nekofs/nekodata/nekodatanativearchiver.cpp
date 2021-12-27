#include "nekodatanativearchiver.h"
#include "nekodataostream.h"
#include "../common/env.h"
#include "../common/utils.h"
#include "../common/sha256.h"
#ifdef _WIN32
#include "../native_win/nativefilesystem.h"
#else
#include "../native_posix/nativefilesystem.h"
#endif

#include <cstring>
#include <sstream>
#include <thread>
#include <functional>
#include <filesystem>

namespace nekofs {
	static inline bool write_buffer(std::shared_ptr<OStream> os, const void* buffer, const int32_t& count)
	{
		return os && count >= 0 && ostream_write(os, buffer, count) == count;
	}
	static inline bool write_uint32(std::shared_ptr<OStream> os, const uint32_t& value)
	{
		uint8_t tmp[4];
		tmp[0] = static_cast<uint8_t>(value >> 24);
		tmp[1] = static_cast<uint8_t>(value >> 16);
		tmp[2] = static_cast<uint8_t>(value >> 8);
		tmp[3] = static_cast<uint8_t>(value >> 0);
		return os && ostream_write(os, tmp, 4) == 4;
	}
	static inline bool write_int32(std::shared_ptr<OStream> os, const int32_t& value)
	{
		return write_uint32(os, static_cast<uint32_t>(value));
	}
	static inline bool write_uint64(std::shared_ptr<OStream> os, const uint64_t& value)
	{
		uint8_t tmp[8];
		tmp[0] = static_cast<uint8_t>(value >> 56);
		tmp[1] = static_cast<uint8_t>(value >> 48);
		tmp[2] = static_cast<uint8_t>(value >> 40);
		tmp[3] = static_cast<uint8_t>(value >> 32);
		tmp[4] = static_cast<uint8_t>(value >> 24);
		tmp[5] = static_cast<uint8_t>(value >> 16);
		tmp[6] = static_cast<uint8_t>(value >> 8);
		tmp[7] = static_cast<uint8_t>(value >> 0);
		return os && ostream_write(os, tmp, 8) == 8;
	}
	static inline bool write_int64(std::shared_ptr<OStream> os, const int64_t& value)
	{
		return write_uint64(os, static_cast<uint64_t>(value));
	}


	NekodataNativeArchiver::FileTask::FileTask(const std::string& path, std::shared_ptr<IStream> is, int64_t index)
	{
		path_ = path;
		is_ = is;
		index_ = index;
	}
	void NekodataNativeArchiver::FileTask::setStatus(Status status)
	{
		std::lock_guard lock(mtx_);
		status_ = status;
	}
	NekodataNativeArchiver::FileTask::Status NekodataNativeArchiver::FileTask::getStatus()
	{
		std::lock_guard lock(mtx_);
		return status_;
	}
	int64_t NekodataNativeArchiver::FileTask::getIndex() const
	{
		return index_;
	}
	std::tuple<int64_t, int64_t> NekodataNativeArchiver::FileTask::getRange() const
	{
		return std::tuple<int64_t, int64_t>(index_ * nekofs_kNekoData_LZ4_Buffer_Size, std::min(is_->getLength(), (index_ + 1) * nekofs_kNekoData_LZ4_Buffer_Size));
	}
	const std::string& NekodataNativeArchiver::FileTask::getPath() const
	{
		return path_;
	}
	std::shared_ptr<IStream> NekodataNativeArchiver::FileTask::getIStream() const
	{
		return is_;
	}
	void NekodataNativeArchiver::FileTask::setBuffer(std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> buffer)
	{
		compressBuffer_ = buffer;
	}
	std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> NekodataNativeArchiver::FileTask::getBuffer() const
	{
		return compressBuffer_;
	}
	void NekodataNativeArchiver::FileTask::setCompressedSize(int32_t size)
	{
		compressedSize_ = size;
	}
	int32_t NekodataNativeArchiver::FileTask::getCompressedSize() const
	{
		return compressedSize_;
	}
	bool NekodataNativeArchiver::FileTask::isFinalTask() const
	{
		return std::get<1>(getRange()) >= is_->getLength();
	}


	NekodataNativeArchiver::NekodataNativeArchiver(const std::string& archiveFilename, int64_t volumeSize)
	{
		volumeSize >>= 20;
		volumeSize <<= 20;
		isStreamMode_ = false;
		archiveFilename_ = archiveFilename;
		volumeSize_ = volumeSize;
		if (!str_EndWith(archiveFilename, nekofs_kNekodata_FileExtension))
		{
			archiveFilename_.append(nekofs_kNekodata_FileExtension);
		}
	}
	NekodataNativeArchiver::NekodataNativeArchiver(const std::string& archiveFilename, std::shared_ptr<OStream> os, int64_t volumeSize)
	{
		volumeSize >>= 20;
		volumeSize <<= 20;
		isStreamMode_ = true;
		rawOS_ = os;
		archiveFilename_ = archiveFilename;
		volumeSize_ = volumeSize;
		if (!str_EndWith(archiveFilename, nekofs_kNekodata_FileExtension))
		{
			archiveFilename_.append(nekofs_kNekodata_FileExtension);
		}
	}
	void NekodataNativeArchiver::addFile(const std::string& filepath, std::shared_ptr<FileSystem> srcfs, const std::string& srcfilepath)
	{
		archiveFileList_[filepath] = std::make_pair(FileCategory::File, std::make_pair(srcfs, srcfilepath));
	}
	void NekodataNativeArchiver::addRawFile(const std::string& filepath, std::shared_ptr<IStream> is)
	{
		archiveFileList_[filepath] = std::make_pair(FileCategory::RawStream, is);
	}
	std::shared_ptr<NekodataNativeArchiver> NekodataNativeArchiver::addArchive(const std::string& filepath, int64_t volumeSize)
	{
		auto newArchiver = std::make_shared<NekodataNativeArchiver>(filepath, os_, volumeSize);
		archiveFileList_[filepath] = std::make_pair<FileCategory, std::any>(FileCategory::Archiver, newArchiver);
		return newArchiver;
	}
	bool NekodataNativeArchiver::archive()
	{
		os_ = std::make_shared<NekodataOStream>(shared_from_this(), volumeSize_);
		return archiveFiles() && archiveCentralDirectory() && archiveFileFooters();
	}
	bool NekodataNativeArchiver::archiveFileHeader(std::shared_ptr<OStream> os)
	{
		return write_buffer(os, nekofs_kNekodata_FileHeader.data(), static_cast<int32_t>(nekofs_kNekodata_FileHeader.size()));
	}
	bool NekodataNativeArchiver::archiveFiles()
	{
		const auto filesCount = archiveFileList_.size();
		const size_t kThreadNum = 12;
		bool hasError = false;
		std::vector<std::thread> t;
		for (size_t i = 0; i < kThreadNum; i++)
		{
			t.push_back(std::thread(std::bind(&NekodataNativeArchiver::threadfunction, shared_from_this())));
		}
		while (!hasError)
		{
			std::string taskpath;
			std::pair<FileCategory, std::any> task;
			{
				std::lock_guard lock(mtx_archiveFileList_);
				if (archiveFileList_.empty())
				{
					break;
				}
				taskpath = archiveFileList_.begin()->first;
				task = archiveFileList_.begin()->second;
			}
			{
				std::stringstream ss;
				ss << u8"pack file ";
				ss << u8"[" << files_.size() + 1 << u8"/" << filesCount << u8"] ";
				ss << taskpath;
				loginfo(ss.str());
			}
			if (task.first == FileCategory::Archiver)
			{
				std::shared_ptr<NekodataNativeArchiver> archiver = std::any_cast<std::shared_ptr<NekodataNativeArchiver>>(task.second);
				archiver->archive();
				std::lock_guard lock(mtx_archiveFileList_);
				archiveFileList_.erase(archiveFileList_.cbegin());
				cond_getTask_.notify_all();
			}
			else if (task.first == FileCategory::File)
			{
				sha256sum hash;
				NekodataFileMeta meta;
				{
					std::pair<std::shared_ptr<FileSystem>, std::string> t = std::any_cast<std::pair<std::shared_ptr<FileSystem>, std::string>>(task.second);
					auto size = t.first->getSize(t.second);
					meta.setOriginalSize(size);
					hasError = size < 0;
				}
				while (!hasError)
				{
					std::shared_ptr<FileTask> ftask;
					{
						std::unique_lock lock(mtx_taskList_);
						while (!ftask && !hasError)
						{
							while (taskList_.empty())
							{
								cond_finishTask_.wait(lock);
							}
							if (taskList_.front()->getStatus() == FileTask::Status::Finish)
							{
								ftask = taskList_.front();
								taskList_.pop();
								cond_getTask_.notify_one();
								if (ftask->isFinalTask())
								{
									std::lock_guard lock(mtx_archiveFileList_);
									archiveFileList_.erase(archiveFileList_.cbegin());
								}
							}
							else if (taskList_.front()->getStatus() == FileTask::Status::Error)
							{
								hasError = true;
							}
						}
					}
					if (ftask)
					{
						hash.update(&(*ftask->getBuffer())[0], ftask->getCompressedSize());
						int32_t actualWrite = ostream_write(os_, &(*ftask->getBuffer())[0], ftask->getCompressedSize());
						if (actualWrite != ftask->getCompressedSize())
						{
							hasError = true;
							break;
						}
						meta.addBlock(ftask->getCompressedSize());
						if (ftask->isFinalTask())
						{
							hash.final();
							meta.setSHA256(hash.readHash());
							files_[ftask->getPath()] = meta;
							break;
						}
					}
				}
			}
			else if (task.first == FileCategory::RawStream)
			{
				std::shared_ptr<IStream> rawis = std::any_cast<std::shared_ptr<IStream>>(task.second);
				auto buffer = env::getInstance().newBuffer4M();
				int32_t actualRead = 0;
				int32_t actualWrite = 0;
				do
				{
					actualWrite = 0;
					actualRead = istream_read(rawis, &(*buffer)[0], static_cast<int32_t>(buffer->size()));
					if (actualRead > 0)
					{
						actualWrite = ostream_write(os_, &(*buffer)[0], actualRead);
					}
				} while (actualRead > 0 && actualRead == actualWrite);
				if (actualRead < 0 || actualRead != actualWrite)
				{
					// error
					break;
				}
				else
				{
					std::lock_guard lock(mtx_archiveFileList_);
					archiveFileList_.erase(archiveFileList_.cbegin());
				}
				cond_getTask_.notify_all();
			}
		}
		{
			std::lock_guard lock(mtx_taskList_);
			finish = true;
		}
		cond_getTask_.notify_all();
		for (size_t i = 0; i < kThreadNum; i++)
		{
			t[i].join();
		}
		t.clear();

		return !hasError && taskList_.empty() && archiveFileList_.empty();
	}
	bool NekodataNativeArchiver::archiveCentralDirectory()
	{
		auto curpos = os_->getPosition();
		bool success = true;
		for (const auto& item : files_)
		{
			success = success && write_uint32(os_, static_cast<uint32_t>(item.first.size()));
			success = success && write_buffer(os_, item.first.c_str(), static_cast<int32_t>(item.first.size()));
			success = success && write_int64(os_, item.second.getOriginalSize());
			success = success && write_int64(os_, item.second.getBlocks().size());
			if (success && !item.second.getBlocks().empty())
			{
				for (const auto& blockSize : item.second.getBlocks())
				{
					success = success && write_int32(os_, blockSize);
				}
			}
		}
		success = success && write_int64(os_, curpos);
		return success;
	}
	bool NekodataNativeArchiver::archiveFileFooters()
	{
		bool success = true;
		for (size_t i = 0; success && i < volumeOS_.size(); i++)
		{
			auto os = std::get<1>(volumeOS_[i]);
			if (os->getLength() == volumeSize_)
			{
				success = success && os->seek(-12, SeekOrigin::End) == volumeSize_ - 12;
			}
			success = success && write_uint32(os_, static_cast<uint32_t>(i));
			success = success && write_uint32(os_, static_cast<uint32_t>(volumeOS_.size()));
			success = success && write_uint32(os_, static_cast<uint32_t>(volumeSize_ >> 20));
		}
		return success;
	}
	std::shared_ptr<NekodataVolumeOStream> NekodataNativeArchiver::getVolumeOStreamByDataPos(int64_t pos)
	{
		int64_t dataSizePerVolume = volumeSize_ - nekofs_kNekodata_FileHeader.size() - 12;
		size_t index = pos / dataSizePerVolume;
		if (pos - index * dataSizePerVolume > 0)
		{
			index++;
		}
		if (index == volumeOS_.size())
		{
			std::string filepath = "";
			{
				std::stringstream ss;
				ss << archiveFilename_.substr(0, archiveFilename_.size() - nekofs_kNekodata_FileExtension.size());
				ss << u8"." << index << nekofs_kNekodata_FileExtension;
				filepath = ss.str();
			}
			if (index > 0 && std::get<1>(volumeOS_[index - 1])->getLength() < volumeSize_)
			{
				if (!std::get<1>(volumeOS_[index - 1])->fill())
				{
					return nullptr;
				}
			}
			if (isStreamMode_)
			{
				volumeOS_.push_back(std::make_tuple(filepath, std::make_shared<NekodataVolumeOStream>(rawOS_, volumeSize_, index * dataSizePerVolume)));
				if (!archiveFileHeader(std::get<1>(volumeOS_[index])))
				{
					return nullptr;
				}
			}
			else
			{
				auto os = env::getInstance().getNativeFileSystem()->openOStream(filepath);
				if (os)
				{
					volumeOS_.push_back(std::make_tuple(filepath, std::make_shared<NekodataVolumeOStream>(os, volumeSize_, index * dataSizePerVolume)));
					if (!archiveFileHeader(std::get<1>(volumeOS_[index])))
					{
						return nullptr;
					}
				}
				else
				{
					return nullptr;
				}
			}
		}
		return std::get<1>(volumeOS_[index]);
	}

	void NekodataNativeArchiver::threadfunction()
	{
		bool needExit = false;
		while (!needExit)
		{
			std::shared_ptr<FileTask> ftask;
			{
				std::unique_lock lock(mtx_taskList_);
				needExit = finish;
				std::pair<std::string, std::pair<FileCategory, std::any>> task;
				while (task.first.empty() && !needExit)
				{
					if (taskList_.size() > 36 && taskList_.front()->getStatus() != FileTask::Status::Error)
					{
						cond_getTask_.wait(lock);
					}
					else if (taskList_.empty())
					{
						std::lock_guard lock(mtx_archiveFileList_);
						if (archiveFileList_.empty())
						{
							needExit = true;
							continue;
						}
						auto it = archiveFileList_.begin();
						task = std::pair<std::string, std::pair<FileCategory, std::any>>(it->first, it->second);
					}
					else
					{
						if (taskList_.front()->getStatus() == FileTask::Status::Error)
						{
							needExit = true;
							continue;
						}
						if (taskList_.back()->isFinalTask())
						{
							// get next ftask
							std::lock_guard lock(mtx_archiveFileList_);
							if (archiveFileList_.empty())
							{
								needExit = true;
								continue;
							}
							else
							{
								auto it = archiveFileList_.find(taskList_.back()->getPath());
								it++;
								if (it != archiveFileList_.end())
								{
									task = std::pair<std::string, std::pair<FileCategory, std::any>>(it->first, it->second);
								}
								else
								{
									needExit = true;
									continue;
								}
							}
						}
						else
						{
							std::lock_guard lock(mtx_archiveFileList_);
							auto it = archiveFileList_.find(taskList_.back()->getPath());
							task = std::pair<std::string, std::pair<FileCategory, std::any>>(it->first, it->second);
						}
					}
					if (task.second.first != FileCategory::File || task.first.empty())
					{
						task.first.clear();
						cond_getTask_.wait(lock);
					}
					else
					{
						// create filetask
						std::pair<std::shared_ptr<FileSystem>, std::string> t = std::any_cast<std::pair<std::shared_ptr<FileSystem>, std::string>>(task.second.second);
						if (taskList_.empty() || taskList_.back()->getPath() != task.first)
						{
							ftask = std::make_shared<FileTask>(task.first, t.first->openIStream(t.second), 0);
						}
						else
						{
							ftask = std::make_shared<FileTask>(task.first, t.first->openIStream(t.second), taskList_.back()->getIndex() + 1);
						}
						taskList_.push(ftask);
					}
					needExit = finish;
				}
				if (needExit)
				{
					continue;
				}
			}
			auto blockBuffer = env::getInstance().newBufferBlockSize();
			auto blockCompressBuffer = env::getInstance().newBufferCompressSize();
			ftask->setBuffer(blockCompressBuffer);
			LZ4_streamHC_t lz4Stream_body;
			LZ4_resetStreamHC(&lz4Stream_body, LZ4HC_CLEVEL_MAX);
			auto range = ftask->getRange();
			int blockSize = static_cast<int>(std::get<1>(range) - std::get<0>(range));
			if (blockSize > 0)
			{
				auto is = ftask->getIStream();
				if (!is)
				{
					// error
					std::stringstream ss;
					ss << u8"sream is null. filepath = ";
					ss << ftask->getPath();
					logerr(ss.str());
					ftask->setStatus(FileTask::Status::Error);
					needExit = true;
					continue;
				}
				if (is->seek(std::get<0>(range), SeekOrigin::Begin) != std::get<0>(range))
				{
					// error
					std::stringstream ss;
					ss << u8"sream seek error. filepath = ";
					ss << ftask->getPath();
					ss << u8", seekpos = ";
					ss << std::get<0>(range);
					logerr(ss.str());
					ftask->setStatus(FileTask::Status::Error);
					needExit = true;
					continue;
				}
				int64_t actualRead = istream_read(is, &(*blockBuffer)[0], blockSize);
				if (actualRead != blockSize)
				{
					// error
					std::stringstream ss;
					ss << u8"read stream error. filepath = ";
					ss << ftask->getPath();
					logerr(ss.str());
					ftask->setStatus(FileTask::Status::Error);
					needExit = true;
					continue;
				}
				const int cmpBytes = LZ4_compress_HC_continue(&lz4Stream_body, (const char*)&(*blockBuffer)[0], (char*)&(*blockCompressBuffer)[0], blockSize, nekofs_kNekoData_LZ4_Compress_Buffer_Size);
				if (cmpBytes <= 0)
				{
					// error
					std::stringstream ss;
					ss << u8"compress error. filepath = ";
					ss << ftask->getPath();
					logerr(ss.str());
					ftask->setStatus(FileTask::Status::Error);
					needExit = true;
					continue;
				}
				ftask->setCompressedSize(cmpBytes);
			}
			else
			{
				ftask->setCompressedSize(0);
			}
			ftask->setStatus(FileTask::Status::Finish);
			cond_finishTask_.notify_one();
		}
		loginfo("thread exit");
	}


	void NekodataFileMeta::setSHA256(const std::array<uint32_t, 8>& sha256)
	{
		sha256_ = sha256;
	}
	const std::array<uint32_t, 8>& NekodataFileMeta::getSHA256() const
	{
		return sha256_;
	}
	void NekodataFileMeta::setCompressedSize(int64_t compressedSize)
	{
		compressedSize_ = compressedSize;
	}
	int64_t NekodataFileMeta::getCompressedSize() const
	{
		return compressedSize_;
	}
	void NekodataFileMeta::setOriginalSize(int64_t originalSize)
	{
		originalSize_ = originalSize;
	}
	int64_t NekodataFileMeta::getOriginalSize() const
	{
		return originalSize_;
	}
	void NekodataFileMeta::addBlock(int32_t blockSize)
	{
		compressedSize_ += blockSize;
		blocks_.push_back(blockSize);
	}
	const std::vector<int32_t>& NekodataFileMeta::getBlocks() const
	{
		return blocks_;
	}
}
