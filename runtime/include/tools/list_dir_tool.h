#pragma once

#include "core/tool.h"

class ListDirTool : public Tool
{
public:
	std::string name() const override { return "list_dir"; }
	json run(const json &arguments) override;
};
