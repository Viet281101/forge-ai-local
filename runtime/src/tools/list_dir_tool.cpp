#include "tools/list_dir_tool.h"
#include <filesystem>

json ListDirTool::run(const json &arguments)
{
	std::string path = arguments.value("path", ".");

	json result = json::array();

	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		result.push_back(entry.path().filename().string());
	}

	return result;
}
