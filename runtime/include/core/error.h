#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class ErrorCode
{
	INVALID_REQUEST,
	INVALID_ARGUMENT,
	UNKNOWN_TOOL,
	TOOL_EXECUTION_FAILED,
	INTERNAL_ERROR
};

inline std::string to_string(ErrorCode code)
{
	switch (code)
	{
	case ErrorCode::INVALID_REQUEST:
		return "INVALID_REQUEST";
	case ErrorCode::INVALID_ARGUMENT:
		return "INVALID_ARGUMENT";
	case ErrorCode::UNKNOWN_TOOL:
		return "UNKNOWN_TOOL";
	case ErrorCode::TOOL_EXECUTION_FAILED:
		return "TOOL_EXECUTION_FAILED";
	default:
		return "INTERNAL_ERROR";
	}
}

inline json make_error(
		ErrorCode code,
		const std::string &message,
		const std::string &field = "",
		const std::string &tool = "")
{
	json err = {
			{"code", to_string(code)},
			{"message", message}};

	if (!field.empty())
		err["field"] = field;
	if (!tool.empty())
		err["tool"] = tool;

	return err;
}
