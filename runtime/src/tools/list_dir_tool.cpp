#include "tools/list_dir_tool.h"

#include <filesystem>

namespace fs = std::filesystem;

json ListDirTool::run(const json &arguments)
{
	std::string path = arguments.value("path", ".");

	json files = json::array();

	try
	{
		for (const auto &entry : fs::directory_iterator(path))
		{
			files.push_back(entry.path().filename().string());
		}
	}
	catch (const std::exception &e)
	{
		return {
				{"error", {{"code", "IO_ERROR"}, {"message", e.what()}}}};
	}

	return {
			{"files", files}};
}
