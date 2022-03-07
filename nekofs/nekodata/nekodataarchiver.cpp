﻿#include "nekodataarchiver.h"
#include "nekodataostream.h"
#include "util.h"
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
	void NekodataNativeArchiver::addFile(const std::string& filepath, const void* buffer, uint32_t length)
	{
		archiveFileList_[filepath] = std::make_pair(FileCategory::Buffer, std::make_pair(buffer, length));
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
		bool success = archiveFiles() && archiveCentralDirectory() && archiveFileFooters();
		os_.reset();
		return success;
	}
	bool NekodataNativeArchiver::archiveFileHeader(std::shared_ptr<OStream> os)
	{
		return ostream_write(os, nekofs_kNekodata_FileHeader.data(), nekofs_kNekodata_FileHeaderSize) == nekofs_kNekodata_FileHeaderSize;
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
				auto beginPos = os_->getPosition();
				if (archiver->archive())
				{
					sha256sum hash;
					NekodataFileMeta meta;
					auto endPos = os_->getPosition();
					auto totalLength = endPos - beginPos;
					if (os_->seek(beginPos, SeekOrigin::Begin) >= 0)
					{
						auto buffer = env::getInstance().newBuffer4M();
						int32_t actualRead = 0;
						do
						{
							int32_t readSize = static_cast<int32_t>(std::min(totalLength, static_cast<int64_t>(buffer->size())));
							actualRead = ostream_read(os_, &(*buffer)[0], readSize);
							if (actualRead > 0)
							{
								totalLength -= actualRead;
								hash.update(&(*buffer)[0], actualRead);
							}
						} while (totalLength > 0 && actualRead > 0);
						if (totalLength == 0)
						{
							hash.final();
							meta.setSHA256(hash.readHash());
							meta.setOriginalSize(endPos - beginPos);
							files_[taskpath] = meta;
							std::lock_guard lock(mtx_archiveFileList_);
							archiveFileList_.erase(archiveFileList_.cbegin());
						}
						else
						{
							hasError = true;
							break;
						}
					}
					else
					{

						hasError = true;
						break;
					}
				}
				else
				{
					hasError = true;
					break;
				}
				cond_getTask_.notify_all();
			}
			else if (task.first == FileCategory::File)
			{
				sha256sum hash;
				NekodataFileMeta meta;
				meta.setBeginPos(os_->getPosition());
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
						if (ftask->getCompressedSize() > 0)
						{
							meta.addBlock(ftask->getCompressedSize());
						}
						if (ftask->isFinalTask())
						{
							hash.final();
							meta.setSHA256(hash.readHash());
							files_[taskpath] = meta;
							break;
						}
					}
				}
			}
			else if (task.first == FileCategory::Buffer)
			{
				sha256sum hash;
				NekodataFileMeta meta;
				meta.setBeginPos(os_->getPosition());
				auto blockCompressBuffer = env::getInstance().newBufferCompressSize();
				std::unique_ptr<LZ4_streamHC_t, std::function<void(LZ4_streamHC_t*)>> lz4Stream_body((LZ4_streamHC_t*)::malloc(sizeof(LZ4_streamHC_t)), [](LZ4_streamHC_t* p) {::free(p); });
				LZ4_resetStreamHC(lz4Stream_body.get(), LZ4HC_CLEVEL_MAX);
				std::pair<const void*, uint32_t> buffer = std::any_cast<std::pair<const void*, uint32_t>>(task.second);
				meta.setOriginalSize(buffer.second);
				int64_t remains = buffer.second;
				const char* blockBuffer = (const char*)buffer.first;
				while (remains > 0)
				{
					int blockSize = (int)std::min(remains, (int64_t)nekofs_kNekoData_LZ4_Buffer_Size);
					remains -= blockSize;
					const int cmpBytes = LZ4_compress_HC_continue(lz4Stream_body.get(), blockBuffer, (char*)&(*blockCompressBuffer)[0], blockSize, nekofs_kNekoData_LZ4_Compress_Buffer_Size);
					if (cmpBytes <= 0)
					{
						// error
						std::stringstream ss;
						ss << u8"FileCategory::Buffer compress error. filepath = ";
						ss << taskpath;
						logerr(ss.str());
						hasError = true;
						break;
					}
					int32_t actualWrite = ostream_write(os_, blockCompressBuffer->data(), cmpBytes);
					if (actualWrite != cmpBytes)
					{
						std::stringstream ss;
						ss << u8"FileCategory::Buffer ostream_write error. filepath = ";
						ss << taskpath;
						ss << u8", cmpBytes = " << cmpBytes;
						logerr(ss.str());
						hasError = true;
						break;
					}
					hash.update(blockCompressBuffer->data(), cmpBytes);
					meta.addBlock(cmpBytes);
					blockBuffer += blockSize;
				}
				hash.final();
				meta.setSHA256(hash.readHash());
				files_[taskpath] = meta;
				std::lock_guard lock(mtx_archiveFileList_);
				archiveFileList_.erase(archiveFileList_.cbegin());
				cond_getTask_.notify_all();
			}
			else if (task.first == FileCategory::RawStream)
			{
				sha256sum hash;
				NekodataFileMeta meta;
				meta.setBeginPos(os_->getPosition());
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
						hash.update(&(*buffer)[0], actualRead);
					}
				} while (actualRead > 0 && actualRead == actualWrite);
				if (actualRead != 0 || actualRead != actualWrite)
				{
					// error
					hasError = true;
					break;
				}
				else
				{
					hash.final();
					meta.setSHA256(hash.readHash());
					meta.setOriginalSize(rawis->getLength());
					files_[taskpath] = meta;
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
			success = success && nekodata_writeString(os_, item.first);
			success = success && nekodata_writeFileSize(os_, item.second.getOriginalSize());
			if (success && item.second.getOriginalSize() > 0)
			{
				success = success && nekodata_writePosition(os_, item.second.getBeginPos());
				success = success && nekodata_writeBlockNum(os_, item.second.getBlocks().size());
				if (success && !item.second.getBlocks().empty())
				{
					for (const auto& blockSize : item.second.getBlocks())
					{
						success = success && nekodata_writeBlockSize(os_, blockSize.second);
					}
				}
				success = success && nekodata_writeSHA256(os_, item.second.getSHA256());
			}
		}
		success = success && nekodata_writeCentralDirectoryPosition(os_, curpos);
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
			success = success && nekodata_writeVolumeNum(os, static_cast<uint32_t>(i + 1));
			success = success && nekodata_writeVolumeNum(os, static_cast<uint32_t>(volumeOS_.size()));
			success = success && nekodata_writeVolumeSize(os, volumeSize_);
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
			if (index != 0)
			{
				std::stringstream ss;
				ss << archiveFilename_.substr(0, archiveFilename_.size() - nekofs_kNekodata_FileExtension.size());
				ss << u8"." << index << nekofs_kNekodata_FileExtension;
				filepath = ss.str();
			}
			else
			{
				filepath = archiveFilename_;
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
		//loginfo("thread exit");
	}
}