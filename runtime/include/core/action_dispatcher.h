#pragma once

#include <nlohmann/json.hpp>
#include "core/tool_registry.h"
#include "llm/llama_engine.h"
#include <future>
#include <memory>

using json = nlohmann::json;

struct ToolTask
{
	std::string call_id;
	std::string tool;
	std::future<json> future;
};

class ActionDispatcher
{
public:
	ActionDispatcher(ToolRegistry &registry, std::shared_ptr<LlamaEngine> llm_engine);

	json dispatch(const json &request);

private:
	ToolRegistry &tool_registry_;
	std::shared_ptr<LlamaEngine> llm_engine_;

	ToolTask submit_tool_call(const json &call);

	json handle_ping(const json &request);
	json handle_infer(const json &request);
	json handle_list_tools(const json &request);
	json handle_generate(const json &request);
	json handle_model_info(const json &request);

	// Helper for AI-powered tool calling
	json infer_with_ai(const json &request);
	bool is_tool_call_response(const std::string &text, json &parsed);
};
