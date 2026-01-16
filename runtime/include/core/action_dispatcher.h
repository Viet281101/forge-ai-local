#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ActionDispatcher
{
public:
	ActionDispatcher() = default;

	json dispatch(const json &request);

private:
	json handle_ping(const json &request);
	json handle_infer(const json &request);
};
