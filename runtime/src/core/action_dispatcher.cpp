#include "core/action_dispatcher.h"
#include "core/error.h"
#include <regex>

ActionDispatcher::ActionDispatcher(
		ToolRegistry &registry,
		std::shared_ptr<LlamaEngine> llm_engine)
		: tool_registry_(registry), llm_engine_(llm_engine)
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
	else if (action == "generate")
	{
		return handle_generate(request);
	}
	else if (action == "model_info")
	{
		return handle_model_info(request);
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

ToolTask ActionDispatcher::submit_tool_call(const json &call)
{
	if (!call.contains("id") || !call.contains("function"))
	{
		throw std::runtime_error("tool_call must contain id and function");
	}

	const auto &fn = call["function"];

	if (!fn.contains("name"))
	{
		throw std::runtime_error("function.name is required");
	}

	std::string call_id = call["id"];
	std::string tool = fn["name"];
	json args = fn.value("arguments", json::object());

	if (!tool_registry_.has(tool))
	{
		throw std::runtime_error("unknown tool: " + tool);
	}

	auto fut = std::async(std::launch::async, [this, tool, args]()
												{
		try {
			return tool_registry_.invoke(tool, args);
		} catch (const std::exception &e) {
			return json{
				{"error", make_error(
					ErrorCode::TOOL_EXECUTION_FAILED,
					e.what(),
					"",
					tool
				)}
			};
		} });

	return {call_id, tool, std::move(fut)};
}

bool ActionDispatcher::is_tool_call_response(const std::string &text, json &parsed)
{
	// Try to extract JSON from text
	// Look for pattern: {"tool":"...", "arguments":{...}}

	std::regex json_pattern(R"(\{[^}]*"tool"[^}]*\})");
	std::smatch match;

	if (std::regex_search(text, match, json_pattern))
	{
		try
		{
			parsed = json::parse(match.str());
			if (parsed.contains("tool") && parsed.contains("arguments"))
			{
				return true;
			}
		}
		catch (...)
		{
			// Not valid JSON
		}
	}

	return false;
}

json ActionDispatcher::infer_with_ai(const json &request)
{
	if (!llm_engine_ || !llm_engine_->is_loaded())
	{
		return error_response(
				"infer",
				make_error(ErrorCode::INTERNAL_ERROR, "LLM engine not available"));
	}

	const auto &messages = request["messages"];

	// Build prompt with available tools
	std::vector<json> chat_messages;

	// Add system message with tool definitions
	json system_msg = {
			{"role", "system"},
			{"content", "You have access to these tools:\n"}};

	auto tools = tool_registry_.list();
	for (const auto &tool : tools)
	{
		const auto &fn = tool["function"];
		system_msg["content"] = system_msg["content"].get<std::string>() +
														"- " + fn["name"].get<std::string>() + ": " +
														fn["description"].get<std::string>() + "\n";
	}

	system_msg["content"] = system_msg["content"].get<std::string>() +
													"\nTo use a tool, respond with JSON: {\"tool\":\"name\",\"arguments\":{...}}";

	chat_messages.push_back(system_msg);

	// Add conversation messages
	for (const auto &msg : messages)
	{
		chat_messages.push_back(msg);
	}

	// Generate response
	int max_tokens = request.value("max_tokens", 512);
	float temperature = request.value("temperature", 0.7f);

	try
	{
		auto result = llm_engine_->chat(chat_messages, max_tokens, temperature);

		// Check if response is a tool call
		json tool_call;
		if (is_tool_call_response(result.text, tool_call))
		{
			// Execute the tool
			std::string tool_name = tool_call["tool"];
			json tool_args = tool_call["arguments"];

			if (!tool_registry_.has(tool_name))
			{
				return error_response(
						"infer",
						make_error(ErrorCode::UNKNOWN_TOOL, "Tool not found: " + tool_name));
			}

			auto tool_result = tool_registry_.invoke(tool_name, tool_args);

			// Generate final response with tool result
			chat_messages.push_back({{"role", "assistant"},
															 {"content", result.text}});

			chat_messages.push_back({{"role", "tool"},
															 {"name", tool_name},
															 {"content", tool_result.dump()}});

			auto final_result = llm_engine_->chat(chat_messages, max_tokens, temperature);

			return {
					{"status", "ok"},
					{"action", "infer"},
					{"result", {{"type", "assistant"}, {"message", {{"role", "assistant"}, {"content", final_result.text}}}, {"tool_used", tool_name}, {"tokens_used", result.tokens_generated + final_result.tokens_generated}, {"tokens_per_second", final_result.tokens_per_second}}}};
		}
		else
		{
			// Direct response, no tool needed
			return {
					{"status", "ok"},
					{"action", "infer"},
					{"result", {{"type", "assistant"}, {"message", {{"role", "assistant"}, {"content", result.text}}}, {"tokens_used", result.tokens_generated}, {"tokens_per_second", result.tokens_per_second}}}};
		}
	}
	catch (const std::exception &e)
	{
		return error_response(
				"infer",
				make_error(ErrorCode::INTERNAL_ERROR, e.what()));
	}
}

json ActionDispatcher::handle_infer(const json &request)
{
	if (!request.contains("messages") || !request["messages"].is_array())
	{
		return error_response(
				"infer",
				make_error(ErrorCode::INVALID_REQUEST, "messages must be an array"));
	}

	// Check if this is explicit tool calling (old style) or AI-powered
	bool has_explicit_tools = false;
	for (const auto &msg : request["messages"])
	{
		if (msg.contains("tool_calls") || msg.contains("tool_call"))
		{
			has_explicit_tools = true;
			break;
		}
	}

	if (has_explicit_tools)
	{
		// Original behavior: execute explicit tool calls
		std::vector<ToolTask> tasks;

		for (const auto &msg : request["messages"])
		{
			if (msg.contains("tool_calls") && msg["tool_calls"].is_array())
			{
				for (const auto &call : msg["tool_calls"])
				{
					try
					{
						tasks.push_back(submit_tool_call(call));
					}
					catch (const std::exception &e)
					{
						return error_response(
								"infer",
								make_error(ErrorCode::INVALID_REQUEST, e.what()));
					}
				}
			}
			else if (msg.contains("tool_call"))
			{
				try
				{
					tasks.push_back(submit_tool_call(msg["tool_call"]));
				}
				catch (const std::exception &e)
				{
					return error_response(
							"infer",
							make_error(ErrorCode::INVALID_REQUEST, e.what()));
				}
			}
		}

		if (tasks.empty())
		{
			return {
					{"status", "ok"},
					{"action", "infer"},
					{"result", {{"type", "assistant"}, {"message", {{"role", "assistant"}, {"content", "No tool call detected"}}}}}};
		}

		json results = json::array();

		for (auto &task : tasks)
		{
			json data;

			try
			{
				data = task.future.get();
			}
			catch (const std::exception &e)
			{
				return error_response(
						"infer",
						make_error(
								ErrorCode::TOOL_EXECUTION_FAILED,
								e.what(),
								"",
								task.tool));
			}

			if (data.contains("error"))
			{
				results.push_back({{"role", "tool"},
													 {"tool_call_id", task.call_id},
													 {"name", task.tool},
													 {"error", data["error"]}});
				continue;
			}

			results.push_back({{"role", "tool"},
												 {"tool_call_id", task.call_id},
												 {"name", task.tool},
												 {"content", data}});
		}

		return {
				{"status", "ok"},
				{"action", "infer"},
				{"result", {{"type", "tool_results"}, {"messages", results}}}};
	}
	else
	{
		// AI-powered mode: let LLM decide what to do
		return infer_with_ai(request);
	}
}

json ActionDispatcher::handle_list_tools(const json &)
{
	return {
			{"status", "ok"},
			{"action", "list_tools"},
			{"result", {{"tools", tool_registry_.list()}}}};
}

json ActionDispatcher::handle_generate(const json &request)
{
	if (!llm_engine_ || !llm_engine_->is_loaded())
	{
		return error_response(
				"generate",
				make_error(ErrorCode::INTERNAL_ERROR, "LLM engine not available"));
	}

	if (!request.contains("prompt"))
	{
		return error_response(
				"generate",
				make_error(ErrorCode::INVALID_REQUEST, "prompt is required"));
	}

	std::string prompt = request["prompt"];
	int max_tokens = request.value("max_tokens", 512);
	float temperature = request.value("temperature", 0.7f);

	std::vector<std::string> stop;
	if (request.contains("stop") && request["stop"].is_array())
	{
		for (const auto &s : request["stop"])
		{
			if (s.is_string())
				stop.push_back(s);
		}
	}

	try
	{
		auto result = llm_engine_->generate(prompt, max_tokens, temperature, stop);

		return {
				{"status", "ok"},
				{"action", "generate"},
				{"result", {{"text", result.text}, {"tokens_generated", result.tokens_generated}, {"tokens_per_second", result.tokens_per_second}, {"stop_reason", result.stop_reason}, {"stopped_by_limit", result.stopped_by_limit}}}};
	}
	catch (const std::exception &e)
	{
		return error_response(
				"generate",
				make_error(ErrorCode::INTERNAL_ERROR, e.what()));
	}
}

json ActionDispatcher::handle_model_info(const json &)
{
	if (!llm_engine_ || !llm_engine_->is_loaded())
	{
		return {
				{"status", "ok"},
				{"action", "model_info"},
				{"result", {{"loaded", false}}}};
	}

	return {
			{"status", "ok"},
			{"action", "model_info"},
			{"result", {{"loaded", true}, {"model_name", llm_engine_->model_name()}, {"context_size", llm_engine_->context_size()}, {"vocab_size", llm_engine_->vocab_size()}}}};
}
