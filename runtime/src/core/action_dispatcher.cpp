#include "core/action_dispatcher.h"
#include "tools/list_dir_tool.h"

ActionDispatcher::ActionDispatcher(ToolRegistry &registry)
		: tool_registry_(registry)
{
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

json ActionDispatcher::handle_infer(const json &request)
{
	if (!request.contains("messages") || !request["messages"].is_array())
	{
		return {
				{"status", "error"},
				{"action", "infer"},
				{"error", {{"code", "INVALID_REQUEST"}, {"message", "messages must be an array"}}}};
	}

	const auto &messages = request["messages"];
	json results = json::array();

	for (const auto &msg : messages)
	{
		if (!msg.contains("tool_call"))
			continue;

		const auto &call = msg["tool_call"];

		if (!call.contains("id") || !call.contains("name"))
		{
			return {
					{"status", "error"},
					{"action", "infer"},
					{"error", {{"code", "INVALID_TOOL_CALL"}, {"message", "tool_call must contain id and name"}}}};
		}

		std::string call_id = call["id"];
		std::string tool = call["name"];
		json args = call.value("arguments", json::object());

		if (!tool_registry_.has(tool))
		{
			return {
					{"status", "error"},
					{"action", "infer"},
					{"error", {{"code", "UNKNOWN_TOOL"}, {"tool", tool}}}};
		}

		json data = tool_registry_.invoke(tool, args);

		// validation / execution error
		if (data.contains("error"))
		{
			return {
					{"status", "error"},
					{"action", "infer"},
					{"error", data["error"]}};
		}

		results.push_back({{"role", "tool"},
											 {"tool_call_id", call_id},
											 {"name", tool},
											 {"content", data}});
	}

	// No tool calls found
	if (results.empty())
	{
		return {
				{"status", "ok"},
				{"action", "infer"},
				{"result", {{"type", "assistant"}, {"message", {{"role", "assistant"}, {"content", "No tool call detected."}}}}}};
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
			{"tools", tool_registry_.list()}};
}
