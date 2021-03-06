#include "nekodataarchiver.h"
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
	NekodataArchiver::FileBlockTask::FileBlockTask(const std::string& path, std::shared_ptr<IStream> is, int64_t index)
	{
		path_ = path;
		is_ = is;
		index_ = index;
	}
	void NekodataArchiver::FileBlockTask::setStatus(Status status)
	{
		std::lock_guard lock(mtx_);
		status_ = status;
	}
	NekodataArchiver::FileBlockTask::Status NekodataArchiver::FileBlockTask::getStatus()
	{
		std::lock_guard lock(mtx_);
		return status_;
	}
	int64_t NekodataArchiver::FileBlockTask::getIndex() const
	{
		return index_;
	}
	std::tuple<int64_t, int64_t> NekodataArchiver::FileBlockTask::getRange() const
	{
		return std::tuple<int64_t, int64_t>(index_ * nekofs_kNekoData_LZ4_Buffer_Size, std::min(is_->getLength(), (index_ + 1) * nekofs_kNekoData_LZ4_Buffer_Size));
	}
	const std::string& NekodataArchiver::FileBlockTask::getPath() const
	{
		return path_;
	}
	std::shared_ptr<IStream> NekodataArchiver::FileBlockTask::getIStream() const
	{
		return is_;
	}
	void NekodataArchiver::FileBlockTask::setBuffer(std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> buffer)
	{
		compressBuffer_ = buffer;
	}
	std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Compress_Buffer_Size>> NekodataArchiver::FileBlockTask::getBuffer() const
	{
		return compressBuffer_;
	}
	void NekodataArchiver::FileBlockTask::setCompressedSize(int32_t size)
	{
		compressedSize_ = size;
	}
	int32_t NekodataArchiver::FileBlockTask::getCompressedSize() const
	{
		return compressedSize_;
	}
	bool NekodataArchiver::FileBlockTask::isFinalTask() const
	{
		return std::get<1>(getRange()) >= is_->getLength();
	}


	NekodataArchiver::NekodataArchiver(const std::string& archiveFilename, int64_t volumeSize, bool streamMode)
	{
		volumeSize >>= 20;
		volumeSize <<= 20;
		isStreamMode_ = streamMode;
		archiveFilename_ = archiveFilename;
		volumeSize_ = volumeSize;
		if (!str_EndWith(archiveFilename, nekofs_kNekodata_FileExtension))
		{
			archiveFilename_.append(nekofs_kNekodata_FileExtension);
		}
	}
	void NekodataArchiver::addFile(const std::string& filepath, std::shared_ptr<FileSystem> srcfs, const std::string& srcfilepath)
	{
		ArchiveInfo_File fileInfo;
		fileInfo.fs = srcfs;
		fileInfo.filepath = srcfilepath;
		fileInfo.length = srcfs->getSize(srcfilepath);
		archiveFileList_[filepath] = std::make_pair(FileCategory::File, fileInfo);
	}
	void NekodataArchiver::addBuffer(const std::string& filepath, const void* buffer, int64_t length)
	{
		ArchiveInfo_Buffer bufferInfo;
		bufferInfo.buffer = buffer;
		bufferInfo.length = length;
		archiveFileList_[filepath] = std::make_pair(FileCategory::Buffer, bufferInfo);
	}
	void NekodataArchiver::addRawFile(const std::string& filepath, std::shared_ptr<IStream> is, const NekodataFileMeta& meta)
	{
		ArchiveInfo_RawNekodataStream streamInfo;
		streamInfo.is = is;
		streamInfo.meta = meta;
		archiveFileList_[filepath] = std::make_pair(FileCategory::RawNekodataStream, streamInfo);
	}
	std::shared_ptr<NekodataArchiver> NekodataArchiver::addArchive(const std::string& filepath)
	{
		auto newArchiver = std::make_shared<NekodataArchiver>(filepath, nekofs_kNekodata_MaxVolumeSize, true);
		archiveFileList_[filepath] = std::make_pair<FileCategory, std::any>(FileCategory::Archiver, newArchiver);
		return newArchiver;
	}
	bool NekodataArchiver::archive(std::function<void()> completeOneCallback)
	{
		completeOneCallback_ = completeOneCallback;
		if (isStreamMode_)
		{
			logerr(u8"can not use stream mode on top-level call!");
			return false;
		}
		os_ = std::make_shared<NekodataOStream>(shared_from_this(), volumeSize_);
		bool success = archiveFiles() && archiveCentralDirectory() && archiveFileFooters();
		os_.reset();
		completeOneCallback_ = nullptr;
		return success;
	}
	bool NekodataArchiver::archive(std::shared_ptr<OStream> os, const std::string& progressInfo, std::function<void()> completeOneCallback)
	{
		progressInfo_ = progressInfo;
		completeOneCallback_ = completeOneCallback;
		rawOS_ = os;
		os_ = std::make_shared<NekodataOStream>(shared_from_this(), volumeSize_);
		bool success = archiveFiles() && archiveCentralDirectory() && archiveFileFooters();
		os_.reset();
		rawOS_.reset();
		completeOneCallback_ = nullptr;
		return success;
	}
	bool NekodataArchiver::archiveFileHeader(std::shared_ptr<OStream> os)
	{
		return ostream_write(os, nekofs_kNekodata_FileHeader.data(), nekofs_kNekodata_FileHeaderSize) == nekofs_kNekodata_FileHeaderSize;
	}
	bool NekodataArchiver::archiveFiles()
	{
		const auto filesCount = archiveFileList_.size();
		const size_t kThreadNum = 3;
		bool hasError = false;
		std::vector<std::thread> t;
		for (size_t i = 0; i < kThreadNum; i++)
		{
			t.push_back(std::thread(std::bind(&NekodataArchiver::threadfunction, shared_from_this())));
		}
		while (!hasError)
		{
			std::string pack_progress;
			std::string taskpath;
			std::pair<FileCategory, std::any>* task = nullptr;
			{
				std::lock_guard lock(mtx_archiveFileList_);
				if (archiveFileList_.empty())
				{
					break;
				}
				taskpath = archiveFileList_.begin()->first;
				task = &archiveFileList_.begin()->second;
			}
			{
				std::stringstream ss;
				ss << u8"[" << files_.size() + 1 << u8"/" << filesCount << u8"] ";
				ss << taskpath;
				pack_progress = ss.str();
			}
			{
				std::stringstream ss;
				ss << u8"pack file ";
				if (!progressInfo_.empty())
				{
					ss << progressInfo_;
					ss << u8" < ";
				}
				ss << pack_progress;
				loginfo(ss.str());
			}
			if (task->first == FileCategory::Archiver)
			{
				std::shared_ptr<NekodataArchiver> archiver = std::any_cast<std::shared_ptr<NekodataArchiver>>(task->second);
				int64_t beginPos = os_->getPosition();
				if (archiver->archive(os_, pack_progress, completeOneCallback_))
				{
					sha256sum hash;
					NekodataFileMeta meta;
					meta.setBeginPos(beginPos);
					int64_t endPos = os_->getPosition();
					int64_t totalLength = endPos - beginPos;
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
							if (completeOneCallback_ != nullptr)
							{
								completeOneCallback_();
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
				}
				else
				{
					hasError = true;
					break;
				}
				cond_getTask_.notify_all();
			}
			else if (task->first == FileCategory::File)
			{
				sha256sum hash;
				NekodataFileMeta meta;
				meta.setBeginPos(os_->getPosition());
				const ArchiveInfo_File& fileInfo = std::any_cast<const ArchiveInfo_File&>(task->second);
				meta.setOriginalSize(fileInfo.length);
				hasError = fileInfo.length < 0;
				while (!hasError)
				{
					// 文件大小如果为0，不需要压缩。
					if (fileInfo.length == 0)
					{
						hash.final();
						meta.setSHA256(hash.readHash());
						files_[taskpath] = meta;
						std::lock_guard lock(mtx_archiveFileList_);
						archiveFileList_.erase(archiveFileList_.cbegin());
						if (completeOneCallback_ != nullptr)
						{
							completeOneCallback_();
						}
						break;
					}
					// 写文件
					std::shared_ptr<FileBlockTask> ftask;
					{
						// 获取FileBlockTask
						std::unique_lock lock(mtx_taskList_);
						while (!ftask && !hasError)
						{
							while (taskList_.empty() || taskList_.front()->getStatus() == FileBlockTask::Status::None)
							{
								cond_finishTask_.wait(lock);
							}
							if (taskList_.front()->getStatus() == FileBlockTask::Status::Finish)
							{
								ftask = taskList_.front();
								taskList_.pop();
								if (ftask->isFinalTask())
								{
									std::lock_guard lock(mtx_archiveFileList_);
									archiveFileList_.erase(archiveFileList_.cbegin());
								}
							}
							else if (taskList_.front()->getStatus() == FileBlockTask::Status::Error)
							{
								hasError = true;
							}
						}
					}
					if (ftask)
					{
						cond_getTask_.notify_one();
						hash.update(&(*ftask->getBuffer())[0], ftask->getCompressedSize());
						int32_t actualWrite = ostream_write(os_, &(*ftask->getBuffer())[0], ftask->getCompressedSize());
						if (actualWrite != ftask->getCompressedSize())
						{
							// 写文件发生错误，中止流程。
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
							if (completeOneCallback_ != nullptr)
							{
								completeOneCallback_();
							}
							break;
						}
					}
				}
			}
			else if (task->first == FileCategory::Buffer)
			{
				const ArchiveInfo_Buffer& buffer = std::any_cast<const ArchiveInfo_Buffer&>(task->second);
				sha256sum hash;
				NekodataFileMeta meta;
				meta.setBeginPos(os_->getPosition());
				if (buffer.length > 0)
				{
					auto blockCompressBuffer = env::getInstance().newBufferCompressSize();
					std::unique_ptr<LZ4_streamHC_t, std::function<void(LZ4_streamHC_t*)>> lz4Stream_body((LZ4_streamHC_t*)::malloc(sizeof(LZ4_streamHC_t)), [](LZ4_streamHC_t* p) {::free(p); });
					meta.setOriginalSize(buffer.length);
					int64_t remains = buffer.length;
					const char* blockBuffer = (const char*)buffer.buffer;
					while (remains > 0)
					{
						int blockSize = (int)std::min(remains, (int64_t)nekofs_kNekoData_LZ4_Buffer_Size);
						remains -= blockSize;
						LZ4_resetStreamHC(lz4Stream_body.get(), LZ4HC_CLEVEL_MAX);
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
				}
				hash.final();
				meta.setSHA256(hash.readHash());
				files_[taskpath] = meta;
				std::lock_guard lock(mtx_archiveFileList_);
				archiveFileList_.erase(archiveFileList_.cbegin());
				if (completeOneCallback_ != nullptr)
				{
					completeOneCallback_();
				}
				cond_getTask_.notify_all();
			}
			else if (task->first == FileCategory::RawNekodataStream)
			{
				ArchiveInfo_RawNekodataStream& streamInfo = std::any_cast<ArchiveInfo_RawNekodataStream&>(task->second);
				streamInfo.meta.setBeginPos(os_->getPosition());
				if (streamInfo.is->getLength() > 0 && (streamInfo.meta.getCompressedSize() == streamInfo.is->getLength() || streamInfo.meta.getOriginalSize() == streamInfo.is->getLength()))
				{
					if (!copyfile(streamInfo.is, os_))
					{
						// error
						logerr(u8"write raw NekodataStream error. filename = " + taskpath);
						hasError = true;
						break;
					}
				}
				else
				{
					if (streamInfo.meta.getCompressedSize() != 0 || streamInfo.meta.getOriginalSize() != 0)
					{
						// error
						logerr(u8"write raw NekodataStream error. invalid meta. filename = " + taskpath);
						hasError = true;
						break;
					}
				}
				files_[taskpath] = streamInfo.meta;
				std::lock_guard lock(mtx_archiveFileList_);
				archiveFileList_.erase(archiveFileList_.cbegin());
				if (completeOneCallback_ != nullptr)
				{
					completeOneCallback_();
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
	bool NekodataArchiver::archiveCentralDirectory()
	{
		int64_t curpos = os_->getPosition();
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
	bool NekodataArchiver::archiveFileFooters()
	{
		bool success = true;
		for (size_t i = 0; success && i < volumeOS_.size(); i++)
		{
			auto os = std::get<1>(volumeOS_[i]);
			if (os->getLength() == volumeSize_)
			{
				success = success && os->seek(-nekofs_kNekodata_FileFooterSize, SeekOrigin::End) == volumeSize_ - nekofs_kNekodata_FileFooterSize;
			}
			success = success && nekodata_writeVolumeNum(os, static_cast<uint32_t>(i + 1));
			success = success && nekodata_writeVolumeNum(os, static_cast<uint32_t>(volumeOS_.size()));
			success = success && nekodata_writeVolumeSize(os, volumeSize_);
		}
		return success;
	}
	/*
	* 获取分卷的OStream。如果volumeOS_已存在，直接返回，不存在就创建新的。新创建的一定是在前一个之后。
	*/
	std::shared_ptr<NekodataVolumeOStream> NekodataArchiver::getVolumeOStreamByDataPos(int64_t pos)
	{
		int64_t dataSizePerVolume = volumeSize_ - nekofs_kNekodata_VolumeFormatSize;
		size_t index = pos / dataSizePerVolume;
		if (index >= volumeOS_.size())
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
				// 补全上一个分卷的内容
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

	/*
	* 压缩文件块的线程。做如下几步操作：
	* 1. 查询哪个文件需要压缩，并且获取待压缩区间
	* 2. 根据压缩区间去读取文件，并进行压缩
	* 3. 标记该文件块已压缩，将压缩后的数据放入队列中
	*
	* 如果压缩过程中出现错误，或者没有待压缩的文件，线程自动退出。
	*/
	void NekodataArchiver::threadfunction()
	{
		bool needExit = false;
		std::unique_ptr<LZ4_streamHC_t, std::function<void(LZ4_streamHC_t*)>> lz4Stream_body((LZ4_streamHC_t*)::malloc(sizeof(LZ4_streamHC_t)), [](LZ4_streamHC_t* p) {::free(p); });
		while (!needExit)
		{
			std::shared_ptr<FileBlockTask> ftask;
			{
				std::unique_lock lock(mtx_taskList_);
				needExit = finish;
				while (!needExit)
				{
					if (!taskList_.empty() && taskList_.front()->getStatus() == FileBlockTask::Status::Error)
					{
						needExit = true;
						continue;
					}
					if (taskList_.size() > 36)
					{
						/*
						* 队列已满
						*/
						cond_getTask_.wait(lock);
						needExit = finish;
						continue;
					}
					// 从archiveFile列表获取压缩任务。
					FileCategory fileCaetgory = FileCategory::None;
					{
						std::lock_guard lock(mtx_archiveFileList_);
						auto it = archiveFileList_.begin();
						while (it != archiveFileList_.end())
						{
							fileCaetgory = it->second.first;
							if (fileCaetgory != FileCategory::File)
							{
								// 暂无文件需要压缩
								break;
							}
							ArchiveInfo_File& fileInfo = std::any_cast<ArchiveInfo_File&>(it->second.second);
							if (fileInfo.compressIndex >= fileInfo.length)
							{
								// 此文件的压缩已全部完成。也可能是文件大小为0，不需要压缩
								it++;
								continue;
							}
							else
							{
								// 查询到压缩任务
								ftask = std::make_shared<FileBlockTask>(it->first, fileInfo.fs->openIStream(fileInfo.filepath), fileInfo.compressIndex / nekofs_kNekoData_LZ4_Buffer_Size);
								fileInfo.compressIndex = std::min(fileInfo.length, fileInfo.compressIndex + nekofs_kNekoData_LZ4_Buffer_Size);
								taskList_.push(ftask);
								break;
							}
						}
						if (it == archiveFileList_.end())
						{
							// 没有文件需要压缩，线程退出
							needExit = true;
							continue;
						}
					}
					if (fileCaetgory != FileCategory::File)
					{
						// 暂无文件需要压缩
						cond_getTask_.wait(lock);
						needExit = finish;
						continue;
					}
					break;
				}
				if (needExit)
				{
					continue;
				}
			}
			auto blockBuffer = env::getInstance().newBufferBlockSize();
			auto blockCompressBuffer = env::getInstance().newBufferCompressSize();
			ftask->setBuffer(blockCompressBuffer);
			LZ4_resetStreamHC(lz4Stream_body.get(), LZ4HC_CLEVEL_MAX);
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
					ftask->setStatus(FileBlockTask::Status::Error);
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
					ftask->setStatus(FileBlockTask::Status::Error);
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
					ftask->setStatus(FileBlockTask::Status::Error);
					needExit = true;
					continue;
				}
				const int cmpBytes = LZ4_compress_HC_continue(lz4Stream_body.get(), (const char*)&(*blockBuffer)[0], (char*)&(*blockCompressBuffer)[0], blockSize, nekofs_kNekoData_LZ4_Compress_Buffer_Size);
				if (cmpBytes <= 0)
				{
					// error
					std::stringstream ss;
					ss << u8"compress error. filepath = ";
					ss << ftask->getPath();
					logerr(ss.str());
					ftask->setStatus(FileBlockTask::Status::Error);
					needExit = true;
					continue;
				}
				ftask->setCompressedSize(cmpBytes);
			}
			else
			{
				ftask->setCompressedSize(0);
			}
			ftask->setStatus(FileBlockTask::Status::Finish);
			cond_finishTask_.notify_one();
		}
		//loginfo("thread exit");
	}
}
