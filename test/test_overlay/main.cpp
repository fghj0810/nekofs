#include "../../nekofs_cli/common.h"

#define RAPIDJSON_NOMEMBERITERATORCLASS 1
#include <rapidjson/rapidjson.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include <cstring>
#include <string>
#include <vector>
#include <iostream>

constexpr const char* test_path1 = u8"D:/test/test_overlay/mod1";
constexpr const char* test_path2 = u8"D:/test/test_overlay/mod2";
constexpr const char* test_path3 = u8"D:/test/test_overlay/mod3";
constexpr const char* test_path4 = u8"D:/test/test_overlay/mod4";
constexpr const char* test_path5 = u8"D:/test/test_overlay/mod5";

int main()
{
	setlog();
	auto overlayfs = nekofs_overlay_Create();
	if (!nekofs_overlay_AddNaitvelayer(overlayfs, test_path1))
	{
		std::cerr << "!nekofs_overlay_AddNaitvelayer(overlayfs, test_path1)";
		return -1;
	}
	if (!nekofs_overlay_AddNaitvelayer(overlayfs, test_path2))
	{
		std::cerr << "!nekofs_overlay_AddNaitvelayer(overlayfs, test_path2)";
		return -1;
	}
	if (!nekofs_overlay_AddNaitvelayer(overlayfs, test_path3))
	{
		std::cerr << "!nekofs_overlay_AddNaitvelayer(overlayfs, test_path3)";
		return -1;
	}
	if (!nekofs_overlay_AddNaitvelayer(overlayfs, test_path4))
	{
		std::cerr << "!nekofs_overlay_AddNaitvelayer(overlayfs, test_path4)";
		return -1;
	}
	if (!nekofs_overlay_AddNaitvelayer(overlayfs, test_path5))
	{
		std::cerr << "!nekofs_overlay_AddNaitvelayer(overlayfs, test_path5)";
		return -1;
	}
	nekofs_overlay_RefreshFileList(overlayfs);
	char* allfilesjson = nullptr;
	auto jsonsize = nekofs_filesystem_GetAllFiles(overlayfs, u8"", &allfilesjson);
	std::string allfiles(allfilesjson, jsonsize);
	if (jsonsize > 0)
	{
		nekofs_Free(allfilesjson);
		allfilesjson = nullptr;

		rapidjson::Document d;
		rapidjson::StringStream ss(allfiles.c_str());
		d.ParseStream(ss);
		for (auto it = d.Begin(); it != d.End(); it++)
		{
			char* uri = nullptr;
			auto uriSize = nekofs_overlay_GetFileURI(overlayfs, it->GetString(), &uri);
			std::string uriStr(uri, uriSize);
			if (uriSize > 0)
			{
				nekofs_Free(uri);
				uri = nullptr;
			}
			std::cout << get_sys_str(uriStr) << std::endl;
		}
	}
	return 0;
}
