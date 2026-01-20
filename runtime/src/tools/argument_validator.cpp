#include "tools/argument_validator.h"

static bool check_type(const json &value, const std::string &type)
{
	if (type == "string")
		return value.is_string();
	if (type == "number")
		return value.is_number();
	if (type == "boolean")
		return value.is_boolean();
	if (type == "object")
		return value.is_object();
	return false;
}

std::optional<ValidationError>
ArgumentValidator::validate(json &args, const json &schema)
{
	if (!schema.is_object())
		return std::nullopt;

	// must be object
	if (!args.is_object())
	{
		return ValidationError{"$", "arguments must be an object"};
	}

	const auto &props = schema.value("properties", json::object());

	// apply defaults
	for (auto &[key, prop] : props.items())
	{
		if (!args.contains(key) && prop.contains("default"))
		{
			args[key] = prop["default"];
		}
	}

	// required fields
	for (auto &req : schema.value("required", json::array()))
	{
		if (!args.contains(req.get<std::string>()))
		{
			return ValidationError{
					req.get<std::string>(),
					"missing required field"};
		}
	}

	// validate fields
	for (auto &[key, value] : args.items())
	{
		if (!props.contains(key))
		{
			return ValidationError{
					key,
					"unknown field"};
		}

		const auto &prop = props[key];
		if (prop.contains("type"))
		{
			if (!check_type(value, prop["type"]))
			{
				return ValidationError{
						key,
						"type mismatch, expected " + prop["type"].get<std::string>()};
			}
		}

		if (prop.contains("enum"))
		{
			bool ok = false;
			for (auto &v : prop["enum"])
				if (v == value)
					ok = true;

			if (!ok)
			{
				return ValidationError{
						key,
						"value not in enum"};
			}
		}
	}

	return std::nullopt;
}
