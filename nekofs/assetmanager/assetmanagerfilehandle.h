#pragma once

#include "../common/typedef.h"

#include <cstdint>
#include <memory>

namespace nekofs {
	class AssetManagerFile;

	class AssetManagerFileHandle final: public FileHandle
	{
	private:
		AssetManagerFileHandle(const AssetManagerFileHandle&) = delete;
		AssetManagerFileHandle(const AssetManagerFileHandle&&) = delete;
		AssetManagerFileHandle& operator=(const AssetManagerFileHandle&) = delete;
		AssetManagerFileHandle& operator=(const AssetManagerFileHandle&&) = delete;

	public:
		AssetManagerFileHandle(std::shared_ptr<AssetManagerFile> fPtr);

	public:
		void print() const override;

	private:
		std::shared_ptr<AssetManagerFile> file_;
	};
}
