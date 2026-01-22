#include "llm/llama_config.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LlamaConfig LlamaConfig::from_file(const std::string &path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		throw std::runtime_error("Cannot open config file: " + path);
	}

	json j;
	file >> j;

	LlamaConfig config;

	if (j.contains("model_path"))
		config.model_path = j["model_path"];
	if (j.contains("n_threads"))
		config.n_threads = j["n_threads"];
	if (j.contains("n_ctx"))
		config.n_ctx = j["n_ctx"];
	if (j.contains("max_tokens"))
		config.max_tokens = j["max_tokens"];
	if (j.contains("temperature"))
		config.temperature = j["temperature"];
	if (j.contains("top_p"))
		config.top_p = j["top_p"];
	if (j.contains("top_k"))
		config.top_k = j["top_k"];
	if (j.contains("verbose"))
		config.verbose = j["verbose"];

	return config;
}

void LlamaConfig::save_to_file(const std::string &path) const
{
	json j;
	j["model_path"] = model_path;
	j["n_threads"] = n_threads;
	j["n_ctx"] = n_ctx;
	j["n_batch"] = n_batch;
	j["max_tokens"] = max_tokens;
	j["temperature"] = temperature;
	j["top_p"] = top_p;
	j["top_k"] = top_k;
	j["repeat_penalty"] = repeat_penalty;
	j["verbose"] = verbose;

	std::ofstream file(path);
	file << j.dump(2);
}
