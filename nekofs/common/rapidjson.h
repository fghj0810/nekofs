﻿#pragma once
#include "typedef.h"

#define RAPIDJSON_NOMEMBERITERATORCLASS 1
//#define DRAPIDJSON_NAMESPACE            nekofs::rapidjson

#include "rapidjson/rapidjson.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

namespace nekofs {
	class JsonInputStream;
	class JsonOutputStream;

#ifdef _WIN32
	typedef rapidjson::GenericStringBuffer<rapidjson::UTF16<>>  JSONStringBuffer;
	typedef rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<>>, rapidjson::UTF16<>, rapidjson::UTF16<>>  JSONStringWriter;
	typedef rapidjson::Writer<JsonOutputStream, rapidjson::UTF16<>, rapidjson::UTF8<>>  JSONFileWriter;
	typedef rapidjson::GenericDocument<rapidjson::UTF16<>> JSONDocument;
	typedef rapidjson::GenericValue<rapidjson::UTF16<>> JSONValue;

#else
	typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>>  JSONStringBuffer;
	typedef rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF8<>>, rapidjson::UTF8<>, rapidjson::UTF8<>>  JSONStringWriter;
	typedef rapidjson::Writer<JsonOutputStream, rapidjson::UTF8<>, rapidjson::UTF8<>>  JSONFileWriter;
	typedef rapidjson::GenericDocument<rapidjson::UTF8<>> JSONDocument;
	typedef rapidjson::GenericValue<rapidjson::UTF8<>> JSONValue;
#endif

	class JsonInputStream
	{
	public:
		typedef char Ch;
		JsonInputStream(std::shared_ptr<IStream> is);

	public:
		Ch Peek();
		Ch Take();
		size_t Tell() const;
		Ch* PutBegin();
		void Put(Ch c);
		void Flush();
		size_t PutEnd(Ch* begin);

	private:
		std::shared_ptr<IStream> is_;
		bool peeked_;
		Ch data_;
	};

	class JsonOutputStream
	{
	public:
		typedef char Ch;

		JsonOutputStream(std::shared_ptr<OStream> os);

	public:
		Ch Peek() const;
		Ch Take();
		size_t Tell() const;
		Ch* PutBegin();
		void Put(Ch c);
		void Flush();
		size_t PutEnd(Ch* begin);

	private:
		std::shared_ptr<OStream> os_;
	};
}
