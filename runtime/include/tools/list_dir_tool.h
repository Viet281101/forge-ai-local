#pragma once

#include "core/tool.h"

class ListDirTool : public Tool
{
public:
	std::string name() const override { return "list_dir"; }

	std::string description() const override
	{
		return "List files and directories at a given path";
	}

	json schema() const override
	{
		return {
				{"type", "object"},
				{"properties", {{"path", {{"type", "string"}, {"description", "Directory path to list"}, {"default", "."}}}}}};
	}

	json run(const json &arguments) override;
};
