#pragma once
#include "../typedef.h"

#define RAPIDJSON_NOMEMBERITERATORCLASS 1
//#define DRAPIDJSON_NAMESPACE            nekofs::rapidjson

#include "rapidjson/rapidjson.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

namespace nekofs {
#ifdef _WIN32
	typedef rapidjson::GenericStringBuffer<rapidjson::UTF16<>>  JSONStringBuffer;
	typedef rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<>>, rapidjson::UTF16<>, rapidjson::UTF16<>>  JSONStringWriter;
	typedef rapidjson::GenericDocument<rapidjson::UTF16<>> JSONDocument;
#else
	typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>>  JSONStringBuffer;
	typedef rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF8<>>, rapidjson::UTF8<>, rapidjson::UTF8<>>  JSONStringWriter;
	typedef rapidjson::GenericDocument<rapidjson::UTF8<>> JSONDocument;
#endif

}
