#pragma once

#include <nlohmann/json.hpp>
#include "core/tool_registry.h"

using json = nlohmann::json;

class ActionDispatcher
{
public:
	explicit ActionDispatcher(ToolRegistry &registry);

	json dispatch(const json &request);

private:
	ToolRegistry &tool_registry_;

	json handle_ping(const json &request);
	json handle_infer(const json &request);
};
