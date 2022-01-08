#pragma once

#include "../common/typedef.h"
#include "nekodatafilemeta.h"

#include <cstdint>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>

namespace nekofs {
	class NekodataFile;
	class NekodataRawIStream;

	class NekodataFileSystem final : public FileSystem, public std::enable_shared_from_this<NekodataFileSystem>
	{
		friend class NekodataFile;
		friend class NekodataRawIStream;
		NekodataFileSystem(const NekodataFileSystem&) = delete;
		NekodataFileSystem(NekodataFileSystem&&) = delete;
		NekodataFileSystem& operator=(const NekodataFileSystem&) = delete;
		NekodataFileSystem& operator=(NekodataFileSystem&&) = delete;
		NekodataFileSystem(std::vector<std::shared_ptr<IStream>> v_is, int64_t volumeSize);

	public:
		std::string getCurrentPath() const override;
		std::vector<std::string> getAllFiles(const std::string& dirpath) const override;
		std::unique_ptr<FileHandle> getFileHandle(const std::string& filepath) override;
		std::shared_ptr<IStream> openIStream(const std::string& filepath) override;
		FileType getFileType(const std::string& path) const override;
		int64_t getSize(const std::string& filepath) const override;
		FileSystemType getFSType() const override;

	public:
		static std::shared_ptr<NekodataFileSystem> createFromNative(const std::string& filepath);
		int64_t getVolumeSzie() const;
		int64_t getVolumeDataSzie() const;

	private:
		bool init();
		std::shared_ptr<IStream> getVolumeIStream(size_t index);
		std::shared_ptr<NekodataRawIStream> openRawIStream(int64_t beginPos, int64_t length);
		static void weakDeleteCallback(std::weak_ptr<NekodataFileSystem> filesystem, NekodataFile* file);
		std::shared_ptr<NekodataFile> openFileInternal(const std::string& filepath);
		void closeFileInternal(const std::string& filepath);

	private:
		std::vector<std::shared_ptr<IStream>> v_is_;
		int64_t volumeSize_;
		std::map<std::string, std::pair<NekodataFile*, NekodataFileMeta>> rawFiles_;
		std::map<std::string, std::weak_ptr<NekodataFile>> files_;
		std::mutex mtx_;
	};
}
