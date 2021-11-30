#pragma once

namespace nekofs {
	class noncopyable
	{
	protected:
		noncopyable() = default;
		~noncopyable() = default;

	protected:
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
	};
}

