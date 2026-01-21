#include "core/action_dispatcher.h"
#include "tools/list_dir_tool.h"
#include "core/error.h"

ActionDispatcher::ActionDispatcher(ToolRegistry &registry)
		: tool_registry_(registry)
{
}

static json error_response(const std::string &action, const json &error)
{
	return {
			{"status", "error"},
			{"action", action},
			{"error", error}};
}

json ActionDispatcher::dispatch(const json &request)
{
	int version = request.value("version", 0);
	std::string action = request.value("action", "");

	if (version != 1)
	{
		return {
				{"status", "error"},
				{"error", "unsupported protocol version"}};
	}

	if (action == "ping")
	{
		return handle_ping(request);
	}
	else if (action == "infer")
	{
		return handle_infer(request);
	}
	else if (action == "list_tools")
	{
		return handle_list_tools(request);
	}

	return {
			{"status", "error"},
			{"error", "unknown action"}};
}

json ActionDispatcher::handle_ping(const json &)
{
	return {
			{"status", "ok"},
			{"action", "ping"},
			{"result", "pong"}};
}

json ActionDispatcher::handle_single_tool_call(const json &call)
{
	if (!call.contains("id") || !call.contains("function"))
	{
		return {
				{"error", make_error(
											ErrorCode::INVALID_REQUEST,
											"tool_call must contain id and function")}};
	}

	const auto &fn = call["function"];

	if (!fn.contains("name"))
	{
		return {
				{"error", make_error(
											ErrorCode::INVALID_REQUEST,
											"function.name is required")}};
	}

	std::string call_id = call["id"];
	std::string tool = fn["name"];
	json args = fn.value("arguments", json::object());

	if (!tool_registry_.has(tool))
	{
		return {
				{"error", make_error(
											ErrorCode::UNKNOWN_TOOL,
											"unknown tool",
											"",
											tool)}};
	}

	json data = tool_registry_.invoke(tool, args);

	if (data.contains("error"))
		return data;

	return {
			{"role", "tool"},
			{"tool_call_id", call_id},
			{"name", tool},
			{"content", data}};
}

json ActionDispatcher::handle_infer(const json &request)
{
	if (!request.contains("messages") || !request["messages"].is_array())
	{
		return {
				{"status", "error"},
				{"action", "infer"},
				{"error", make_error(
											ErrorCode::INVALID_REQUEST,
											"messages must be an array")}};
	}

	json results = json::array();

	for (const auto &msg : request["messages"])
	{
		if (msg.contains("tool_calls") && msg["tool_calls"].is_array())
		{
			for (const auto &call : msg["tool_calls"])
			{
				auto res = handle_single_tool_call(call);
				if (res.contains("error"))
					return error_response("infer", res["error"]);

				results.push_back(res);
			}
		}

		else if (msg.contains("tool_call"))
		{
			auto res = handle_single_tool_call(msg["tool_call"]);
			if (res.contains("error"))
				return error_response("infer", res["error"]);

			results.push_back(res);
		}
	}

	if (results.empty())
	{
		return {
				{"status", "ok"},
				{"action", "infer"},
				{"result", {{"type", "assistant"}, {"message", {{"role", "assistant"}, {"content", "No tool call detected"}}}}}};
	}

	return {
			{"status", "ok"},
			{"action", "infer"},
			{"result", {{"type", "tool_results"}, {"messages", results}}}};
}

json ActionDispatcher::handle_list_tools(const json &)
{
	return {
			{"status", "ok"},
			{"action", "list_tools"},
			{"result", {{"tools", tool_registry_.list()}}}};
}
