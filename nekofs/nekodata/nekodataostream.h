#pragma once

#include "../common/typedef.h"

#include <cstdint>
#include <string>
#include <memory>
#include <map>

namespace nekofs {
	class NekodataArchiver;

	class NekodataVolumeOStream final : public OStream, public std::enable_shared_from_this<NekodataVolumeOStream>
	{
		NekodataVolumeOStream(const NekodataVolumeOStream&) = delete;
		NekodataVolumeOStream& operator=(const NekodataVolumeOStream&) = delete;
		NekodataVolumeOStream(NekodataVolumeOStream&&) = delete;
		NekodataVolumeOStream& operator=(NekodataVolumeOStream&&) = delete;
	public:
		NekodataVolumeOStream(std::shared_ptr<OStream> os, int64_t volumeSize, int64_t voldataBeginPos);
		int64_t getVolDataBeginPos() const;
		int64_t getVolDataEndPos_max() const;
		bool fill();

	public:
		int32_t read(void* buf, int32_t size) override;
		int32_t write(const void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;

	private:
		std::shared_ptr<OStream> os_;
		int64_t volumeSize_ = nekofs_kNekodata_MaxVolumeSize;
		int64_t voldataBeginPos_ = 0;
		int64_t position_ = 0;
		int64_t length_ = 0;
		int64_t rawBeginPos_ = 0;
	};

	/*
	* nekodata写数据流。不包含分卷的头尾信息。
	*/
	class NekodataOStream final : public OStream, public std::enable_shared_from_this<NekodataOStream>
	{
		NekodataOStream(const NekodataOStream&) = delete;
		NekodataOStream& operator=(const NekodataOStream&) = delete;
		NekodataOStream(NekodataOStream&&) = delete;
		NekodataOStream& operator=(NekodataOStream&&) = delete;
	public:
		NekodataOStream(std::shared_ptr<NekodataArchiver> archiver, int64_t volumeSize);

	public:
		int32_t read(void* buf, int32_t size) override;
		int32_t write(const void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;

	private:
		std::shared_ptr<NekodataVolumeOStream> prepareVolumeOStream();

	private:
		std::shared_ptr<NekodataArchiver> archiver_;
		int64_t volumeSize_ = nekofs_kNekodata_MaxVolumeSize;
		int64_t position_ = 0;
		int64_t length_ = 0;
		std::shared_ptr<NekodataVolumeOStream> os_;
	};
}
