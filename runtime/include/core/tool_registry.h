#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "core/tool.h"

using json = nlohmann::json;

class ToolRegistry
{
public:
	void register_tool(std::unique_ptr<Tool> tool);
	bool has(const std::string &name) const;
	json invoke(const std::string &name, json arguments) const;
	json list() const;

private:
	std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
};
