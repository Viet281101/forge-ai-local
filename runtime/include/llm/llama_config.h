#pragma once

#include <string>
#include <vector>

struct LlamaConfig
{
	// Model path
	std::string model_path;

	// CPU optimization
	int n_threads = 4;
	int n_threads_batch = 4;

	// Context and memory
	int n_ctx = 2048;
	int n_batch = 512;
	int n_ubatch = 512;
	bool use_mmap = true;
	bool use_mlock = false;

	// Generation defaults
	int max_tokens = 512;
	float temperature = 0.7f;
	float top_p = 0.9f;
	int top_k = 40;
	float repeat_penalty = 1.1f;

	// Stop sequences
	std::vector<std::string> stop_sequences = {"\n\n", "###"};

	// System prompt for tool calling
	std::string system_prompt =
			"You are a helpful AI assistant with access to tools. "
			"When you need to use a tool, respond with JSON in this format:\n"
			"{\"tool\":\"tool_name\",\"arguments\":{...}}\n"
			"Only use tools when necessary.";

	// Logging
	bool verbose = false;

	static LlamaConfig from_file(const std::string &path);
	void save_to_file(const std::string &path) const;
};
