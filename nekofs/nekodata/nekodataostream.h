#pragma once

#include "../common/typedef.h"

#include <cstdint>
#include <string>
#include <memory>
#include <map>

namespace nekofs {
	class NekodataOStream final : public OStream, public std::enable_shared_from_this<NekodataOStream>
	{
		NekodataOStream(const NekodataOStream&) = delete;
		NekodataOStream& operator=(const NekodataOStream&) = delete;
		NekodataOStream(NekodataOStream&&) = delete;
		NekodataOStream& operator=(NekodataOStream&&) = delete;
	public:
		NekodataOStream() = default;

	public:
		int32_t write(const void* buf, int32_t size) override;
		int64_t seek(int64_t offset, const SeekOrigin& origin) override;
		int64_t getPosition() const override;
		int64_t getLength() const override;

	private:
		std::shared_ptr<FileSystem> filesystem_;
		std::string currentpath_;
	};
}
