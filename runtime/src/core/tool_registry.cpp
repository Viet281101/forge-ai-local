#include "core/tool_registry.h"
#include "tools/argument_validator.h"

void ToolRegistry::register_tool(std::unique_ptr<Tool> tool)
{
	tools_[tool->name()] = std::move(tool);
}

bool ToolRegistry::has(const std::string &name) const
{
	return tools_.count(name) > 0;
}

json ToolRegistry::invoke(const std::string &name, json arguments) const
{
	auto it = tools_.find(name);
	if (it == tools_.end())
	{
		return {
				{"error", {{"code", "UNKNOWN_TOOL"}, {"message", "tool not found"}}}};
	}

	auto &tool = it->second;
	const json &schema = tool->schema();

	if (auto err = ArgumentValidator::validate(arguments, schema))
	{
		return {
				{"error", {{"code", "INVALID_ARGUMENT"}, {"field", err->field}, {"message", err->message}}}};
	}

	return tool->run(arguments);
}

json ToolRegistry::list() const
{
	json out = json::array();

	for (const auto &[_, tool] : tools_)
	{
		out.push_back({{"name", tool->name()},
									 {"description", tool->description()},
									 {"schema", tool->schema()}});
	}

	return out;
}
