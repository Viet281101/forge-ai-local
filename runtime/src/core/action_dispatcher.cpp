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
	if (messages.empty())
	{
		return {
				{"status", "error"},
				{"action", "infer"},
				{"error", {{"code", "INVALID_REQUEST"}, {"message", "messages must not be empty"}}}};
	}

	bool has_user = false;
	for (const auto &msg : messages)
	{
		if (!msg.contains("role"))
		{
			return {
					{"status", "error"},
					{"action", "infer"},
					{"error", {{"code", "INVALID_MESSAGE"}, {"message", "each message requires role"}}}};
		}

		if (msg["role"] == "user")
			has_user = true;
	}

	// ===== TOOL CALL HANDLING =====
	for (const auto &msg : messages)
	{
		if (msg.contains("tool_call"))
		{
			const auto &call = msg["tool_call"];

			std::string tool_name = call.value("name", "");
			json arguments = call.value("arguments", json::object());

			if (!tool_registry_.has(tool_name))
			{
				return {
						{"status", "error"},
						{"action", "infer"},
						{"error", {{"code", "UNKNOWN_TOOL"}, {"message", tool_name}}}};
			}

			json tool_result = tool_registry_.invoke(tool_name, arguments);

			return {
					{"status", "ok"},
					{"action", "infer"},
					{"result", {{"role", "tool"}, {"name", tool_name}, {"content", tool_result.dump()}}}};
		}
	}

	// ===== STUB LLM FALLBACK =====
	if (!has_user)
	{
		return {
				{"status", "error"},
				{"action", "infer"},
				{"error", {{"code", "INVALID_REQUEST"}, {"message", "at least one user message is required"}}}};
	}

	std::string last_user_input;
	for (auto it = messages.rbegin(); it != messages.rend(); ++it)
	{
		if ((*it)["role"] == "user")
		{
			last_user_input = (*it)["content"];
			break;
		}
	}

	return {
			{"status", "ok"},
			{"action", "infer"},
			{"result", {{"role", "assistant"}, {"content", "echo: " + last_user_input}}},
			{"usage", {{"prompt_tokens", 0}, {"completion_tokens", 0}}}};
}
