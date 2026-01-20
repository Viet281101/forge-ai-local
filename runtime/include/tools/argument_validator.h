#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <optional>

using json = nlohmann::json;

struct ValidationError
{
	std::string field;
	std::string message;
};

class ArgumentValidator
{
public:
	static std::optional<ValidationError>
	validate(json &args, const json &schema);
};
