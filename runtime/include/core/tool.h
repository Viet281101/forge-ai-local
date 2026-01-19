#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Tool
{
public:
	virtual ~Tool() = default;

	virtual std::string name() const = 0;
	virtual json run(const json &arguments) = 0;
};
