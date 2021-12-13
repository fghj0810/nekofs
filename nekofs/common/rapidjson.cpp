#include "rapidjson.h"
#include "error.h"

namespace nekofs {

	JsonInputStream::JsonInputStream(std::shared_ptr<IStream> is)
	{
		is_ = is;
		peeked_ = false;
		data_ = '\0';
	}

	JsonInputStream::Ch JsonInputStream::Peek()
	{
		if (peeked_)
		{
			return data_;
		}
		else
		{
			if (is_->read(&data_, 1) == 1)
			{
				peeked_ = true;
				return data_;
			}
		}
		return '\0';
	}

	JsonInputStream::Ch JsonInputStream::Take()
	{
		if (peeked_)
		{
			peeked_ = false;
			return data_;
		}
		else
		{
			if (is_->read(&data_, 1) == 1)
			{
				return data_;
			}
		}
		return '\0';
	}

	size_t JsonInputStream::Tell() const
	{
		if (peeked_)
		{
			return is_->getPosition() - 1;
		}
		return is_->getPosition();
	}

	JsonInputStream::Ch* JsonInputStream::PutBegin()
	{
		return nullptr;
	}

	void JsonInputStream::Put(Ch c)
	{

	}

	void JsonInputStream::Flush()
	{

	}

	size_t JsonInputStream::PutEnd(Ch* begin)
	{
		return 0;
	}


	JsonOutputStream::JsonOutputStream(std::shared_ptr<OStream> os)
	{
		os_ = os;
	}

	JsonOutputStream::Ch JsonOutputStream::Peek() const
	{
		return '\0';
	}

	JsonOutputStream::Ch JsonOutputStream::Take()
	{
		return '\0';
	}

	size_t JsonOutputStream::Tell() const
	{
		return 0;
	}

	JsonOutputStream::Ch* JsonOutputStream::PutBegin()
	{
		return nullptr;
	}

	void JsonOutputStream::Put(Ch c)
	{
		if (1 != os_->write(&c, 1))
		{
			throw FSException(FSError::WriteErr);
		}
	}

	void JsonOutputStream::Flush()
	{
	}

	size_t JsonOutputStream::PutEnd(Ch* begin)
	{
		return 0;
	}
}
