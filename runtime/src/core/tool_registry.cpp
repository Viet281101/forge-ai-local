#include "core/tool_registry.h"
#include "tools/argument_validator.h"
#include "core/error.h"

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
				{"error", make_error(
											ErrorCode::UNKNOWN_TOOL,
											"tool not found",
											"",
											name)}};
	}

	auto &tool = it->second;
	const json &schema = tool->schema();

	if (auto err = ArgumentValidator::validate(arguments, schema))
	{
		return {
				{"error", make_error(
											ErrorCode::INVALID_ARGUMENT,
											err->message,
											err->field,
											name)}};
	}

	return tool->run(arguments);
}

json ToolRegistry::list() const
{
	json tools = json::array();

	for (const auto &[_, tool] : tools_)
	{
		tools.push_back({{"type", "function"},
										 {"function", {{"name", tool->name()}, {"description", tool->description()}, {"parameters", tool->schema()}}}});
	}

	return tools;
}
