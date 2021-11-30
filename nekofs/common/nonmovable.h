#pragma once

namespace nekofs {
	class nonmovable
	{
	protected:
		nonmovable() = default;
		~nonmovable() = default;

	protected:
		nonmovable(nonmovable&&) = delete;
		nonmovable& operator=(nonmovable&&) = delete;
	};
}

