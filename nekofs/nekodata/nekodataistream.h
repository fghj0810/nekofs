#pragma once

#include "../common/typedef.h"
#include "../common/lz4.h"

#include <cstdint>
#include <string>
#include <memory>
#include <map>
#include <array>

namespace nekofs {
	class NekodataFileSystem;
	class NekodataFile;

	/*
	* nekodata读数据流。不包含分卷的头尾信息。
	*/
	class NekodataRawIStream final : public IStream, public std::enable_shared_from_this<NekodataRawIStream>
	{
		NekodataRawIStream(const NekodataRawIStream&) = delete;
		NekodataRawIStream(NekodataRawIStream&&) = delete;
		NekodataRawIStream& operator=(const NekodataRawIStream&) = delete;
		NekodataRawIStream& operator=(NekodataRawIStream&&) = delete;
	public:
		NekodataRawIStream(std::shared_ptr<NekodataFileSystem> fs, int64_t beginPos, int64_t length);
	private:
		std::shared_ptr<IStream> prepare();

	public:
		int32_t read(void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;
		std::shared_ptr<IStream> createNew() override;

	private:
		std::shared_ptr<NekodataFileSystem> fs_;
		std::shared_ptr<IStream> is_; // 分卷IStream
		std::pair<int64_t, int64_t> voldataRange; // 分卷包含的数据区间
		int64_t beginPos_ = 0;   // 数据流的起始位置
		int64_t length_ = 0;     // 数据流的长度
		int64_t position_ = 0;   // 相对数据流的起始位置的偏移
	};

	/*
	* nekodata读数据流。解压后的数据。
	*/
	class NekodataIStream final : public IStream, public std::enable_shared_from_this<NekodataIStream>
	{
		NekodataIStream(const NekodataIStream&) = delete;
		NekodataIStream(NekodataIStream&&) = delete;
		NekodataIStream& operator=(const NekodataIStream&) = delete;
		NekodataIStream& operator=(NekodataIStream&&) = delete;
	public:
		NekodataIStream(std::shared_ptr<NekodataFile> file);
	private:
		std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> prepare();

	public:
		int32_t read(void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;
		std::shared_ptr<IStream> createNew() override;

	private:
		std::shared_ptr<NekodataFile> file_;
		std::shared_ptr<std::array<uint8_t, nekofs_kNekoData_LZ4_Buffer_Size>> block_;
		int64_t blockBeginPos_ = 0;
		int64_t blockEndPos_ = 0;
		int64_t position_ = 0;
	};
}
