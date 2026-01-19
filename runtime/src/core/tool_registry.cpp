#include "core/tool_registry.h"

void ToolRegistry::register_tool(std::unique_ptr<Tool> tool)
{
	tools_[tool->name()] = std::move(tool);
}

bool ToolRegistry::has(const std::string &name) const
{
	return tools_.count(name) > 0;
}

json ToolRegistry::invoke(const std::string &name, const json &arguments) const
{
	auto it = tools_.find(name);
	if (it == tools_.end())
	{
		return {
				{"error", "tool not found"}};
	}

	return it->second->run(arguments);
}
