#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "llm/llama_config.h"

// Forward declarations from llama.cpp
struct llama_model;
struct llama_context;
struct llama_sampler;

using json = nlohmann::json;

struct GenerateResult
{
	std::string text;
	int tokens_generated;
	float tokens_per_second;
	bool stopped_by_limit;
	std::string stop_reason;
};

class LlamaEngine
{
public:
	explicit LlamaEngine(const LlamaConfig &config);
	~LlamaEngine();

	// Disable copy
	LlamaEngine(const LlamaEngine &) = delete;
	LlamaEngine &operator=(const LlamaEngine &) = delete;

	// Load model
	bool load();
	void unload();
	bool is_loaded() const { return model_ != nullptr; }

	// Text generation
	GenerateResult generate(
			const std::string &prompt,
			int max_tokens = -1,
			float temperature = -1.0f,
			const std::vector<std::string> &stop = {});

	// Chat completion (with conversation history)
	GenerateResult chat(
			const std::vector<json> &messages,
			int max_tokens = -1,
			float temperature = -1.0f);

	// Get model info
	std::string model_name() const;
	int context_size() const;
	int vocab_size() const;

private:
	LlamaConfig config_;
	llama_model *model_;
	llama_context *ctx_;
	llama_sampler *sampler_;

	// Helper methods
	bool ensure_context();
	void reset_context();
	std::vector<int> tokenize(const std::string &text, bool add_bos = true);
	std::string detokenize(const std::vector<int> &tokens);
	std::string build_chat_prompt(const std::vector<json> &messages);
	void init_sampler(float temperature);
	bool check_stop_sequence(const std::string &text, const std::vector<std::string> &stops);
};
