#include "llm/llama_engine.h"
#include "llama.h"
#include <iostream>
#include <chrono>
#include <sstream>

LlamaEngine::LlamaEngine(const LlamaConfig &config)
		: config_(config), model_(nullptr), ctx_(nullptr), sampler_(nullptr)
{
}

LlamaEngine::~LlamaEngine()
{
	unload();
}

bool LlamaEngine::load()
{
	if (is_loaded())
	{
		std::cerr << "[LlamaEngine] Already loaded\n";
		return true;
	}

	std::cout << "[LlamaEngine] Loading model: " << config_.model_path << "\n";

	// Initialize llama backend
	llama_backend_init();
	llama_numa_init(GGML_NUMA_STRATEGY_DISABLED);

	// Suppress llama.cpp logs unless verbose
	if (!config_.verbose)
	{
		llama_log_set(nullptr, nullptr);
	}

	// Model parameters
	llama_model_params model_params = llama_model_default_params();
	model_params.use_mmap = config_.use_mmap;
	model_params.use_mlock = config_.use_mlock;

	// Load model
	model_ = llama_load_model_from_file(config_.model_path.c_str(), model_params);
	if (!model_)
	{
		std::cerr << "[LlamaEngine] Failed to load model\n";
		return false;
	}

	// Initialize sampler
	init_sampler(config_.temperature);

	std::cout << "[LlamaEngine] Model loaded successfully\n";
	std::cout << "[LlamaEngine] Threads: " << config_.n_threads << "\n";

	// Note: context will be created on first generation
	return true;
}

void LlamaEngine::unload()
{
	if (sampler_)
	{
		llama_sampler_free(sampler_);
		sampler_ = nullptr;
	}

	if (ctx_)
	{
		llama_free(ctx_);
		ctx_ = nullptr;
	}

	if (model_)
	{
		llama_free_model(model_);
		model_ = nullptr;
	}

	llama_backend_free();
}

bool LlamaEngine::ensure_context()
{
	if (ctx_)
		return true;

	// Create context lazily
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = config_.n_ctx;
	ctx_params.n_batch = config_.n_batch;
	ctx_params.n_ubatch = config_.n_ubatch;
	ctx_params.n_threads = config_.n_threads;
	ctx_params.n_threads_batch = config_.n_threads_batch;

	ctx_ = llama_new_context_with_model(model_, ctx_params);
	if (!ctx_)
	{
		std::cerr << "[LlamaEngine] Failed to create context\n";
		return false;
	}

	if (config_.verbose)
	{
		std::cout << "[LlamaEngine] Context created: " << config_.n_ctx << " tokens\n";
	}

	return true;
}

void LlamaEngine::reset_context()
{
	// Recreate context to clear state
	if (ctx_)
	{
		llama_free(ctx_);
		ctx_ = nullptr;
	}
	ensure_context();
}

void LlamaEngine::init_sampler(float temperature)
{
	if (sampler_)
		llama_sampler_free(sampler_);

	float temp = temperature > 0 ? temperature : config_.temperature;

	sampler_ = llama_sampler_chain_init(llama_sampler_chain_default_params());

	llama_sampler_chain_add(sampler_, llama_sampler_init_top_k(config_.top_k));
	llama_sampler_chain_add(sampler_, llama_sampler_init_top_p(config_.top_p, 1));
	llama_sampler_chain_add(sampler_, llama_sampler_init_temp(temp));
	llama_sampler_chain_add(sampler_, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}

std::vector<int> LlamaEngine::tokenize(const std::string &text, bool add_bos)
{
	const llama_vocab *vocab = llama_model_get_vocab(model_);

	int n_tokens = text.length() + (add_bos ? 1 : 0) + 1;
	std::vector<int> tokens(n_tokens);

	n_tokens = llama_tokenize(
			vocab,
			text.c_str(),
			text.length(),
			tokens.data(),
			tokens.size(),
			add_bos,
			false);

	if (n_tokens < 0)
	{
		tokens.resize(-n_tokens);
		n_tokens = llama_tokenize(
				vocab,
				text.c_str(),
				text.length(),
				tokens.data(),
				tokens.size(),
				add_bos,
				false);
	}

	tokens.resize(n_tokens);
	return tokens;
}

std::string LlamaEngine::detokenize(const std::vector<int> &tokens)
{
	const llama_vocab *vocab = llama_model_get_vocab(model_);

	std::string result;
	result.reserve(tokens.size() * 4);

	for (int token : tokens)
	{
		char buf[128];
		int n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, false);
		if (n > 0)
			result.append(buf, n);
	}

	return result;
}

bool LlamaEngine::check_stop_sequence(
		const std::string &text,
		const std::vector<std::string> &stops)
{
	for (const auto &stop : stops)
	{
		if (text.length() >= stop.length())
		{
			if (text.substr(text.length() - stop.length()) == stop)
				return true;
		}
	}
	return false;
}

GenerateResult LlamaEngine::generate(
		const std::string &prompt,
		int max_tokens,
		float temperature,
		const std::vector<std::string> &stop)
{
	if (!model_)
	{
		throw std::runtime_error("Model not loaded");
	}

	GenerateResult result;
	result.tokens_generated = 0;
	result.stopped_by_limit = false;
	result.stop_reason = "completed";

	try
	{
		// Ensure we have a fresh context for each generation
		// This is necessary because transitional llama.cpp lacks proper KV cache clear APIs
		reset_context();
		if (!ctx_)
		{
			throw std::runtime_error("Failed to create context");
		}

		int max_gen = max_tokens > 0 ? max_tokens : config_.max_tokens;
		auto stops = stop.empty() ? config_.stop_sequences : stop;

		// Reinit sampler if temperature changed
		if (temperature > 0 && std::abs(temperature - config_.temperature) > 0.01f)
		{
			init_sampler(temperature);
		}

		// Tokenize prompt
		auto tokens = tokenize(prompt, true);

		if (config_.verbose)
		{
			std::cout << "[LlamaEngine] Prompt tokens: " << tokens.size() << "\n";
		}

		auto start_time = std::chrono::high_resolution_clock::now();

		// Evaluate prompt in batches
		for (size_t i = 0; i < tokens.size(); i += config_.n_batch)
		{
			size_t n_eval = std::min((size_t)config_.n_batch, tokens.size() - i);

			if (llama_decode(ctx_, llama_batch_get_one(&tokens[i], n_eval)))
			{
				throw std::runtime_error("Failed to evaluate prompt");
			}
		}

		const llama_vocab *vocab = llama_model_get_vocab(model_);

		// Generate tokens
		std::vector<int> generated_tokens;
		std::string generated_text;

		for (int i = 0; i < max_gen; ++i)
		{
			int token = llama_sampler_sample(sampler_, ctx_, -1);

			// Check for EOS
			if (llama_vocab_is_eog(vocab, token))
			{
				result.stop_reason = "eos";
				break;
			}

			generated_tokens.push_back(token);

			// Decode token
			char buf[128];
			int n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, false);
			if (n > 0)
			{
				generated_text.append(buf, n);
			}

			// Check stop sequences
			if (check_stop_sequence(generated_text, stops))
			{
				result.stop_reason = "stop_sequence";
				break;
			}

			// Evaluate next token
			if (llama_decode(ctx_, llama_batch_get_one(&token, 1)))
			{
				throw std::runtime_error("Failed to evaluate token");
			}

			result.tokens_generated++;

			if (i == max_gen - 1)
			{
				result.stopped_by_limit = true;
				result.stop_reason = "length";
			}
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

		result.text = generated_text;
		result.tokens_per_second = result.tokens_generated / (duration.count() / 1000.0f);

		if (config_.verbose)
		{
			std::cout << "[LlamaEngine] Generated " << result.tokens_generated
								<< " tokens in " << duration.count() << "ms"
								<< " (" << result.tokens_per_second << " t/s)\n";
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "[LlamaEngine] Generation error: " << e.what() << "\n";
		result.text = "";
		result.stop_reason = "error";
		// Don't rethrow - return error result instead
	}

	return result;
}

std::string LlamaEngine::build_chat_prompt(const std::vector<json> &messages)
{
	std::ostringstream oss;

	oss << config_.system_prompt << "\n\n";

	for (const auto &msg : messages)
	{
		std::string role = msg.value("role", "user");
		std::string content = msg.value("content", "");

		if (role == "system")
		{
			oss << "System: " << content << "\n\n";
		}
		else if (role == "user")
		{
			oss << "User: " << content << "\n\n";
		}
		else if (role == "assistant")
		{
			oss << "Assistant: " << content << "\n\n";
		}
	}

	oss << "Assistant:";
	return oss.str();
}

GenerateResult LlamaEngine::chat(
		const std::vector<json> &messages,
		int max_tokens,
		float temperature)
{
	std::string prompt = build_chat_prompt(messages);
	return generate(prompt, max_tokens, temperature);
}

std::string LlamaEngine::model_name() const
{
	if (!model_)
		return "not loaded";

	char buf[256];
	llama_model_desc(model_, buf, sizeof(buf));
	return std::string(buf);
}

int LlamaEngine::context_size() const
{
	return config_.n_ctx;
}

int LlamaEngine::vocab_size() const
{
	if (!model_)
		return 0;

	const llama_vocab *vocab = llama_model_get_vocab(model_);
	return llama_vocab_n_tokens(vocab);
}
