#include <iostream>
#include <memory>
#include <csignal>
#include <getopt.h>
#include "ipc/socket_server.h"
#include "core/tool_registry.h"
#include "tools/list_dir_tool.h"
#include "llm/llama_engine.h"
#include "llm/llama_config.h"

// Global pointer for signal handling
SocketServer *g_server = nullptr;

void signal_handler(int signum)
{
	std::cout << "\n[forge-runtime] Caught signal " << signum << ", shutting down...\n";
	if (g_server)
	{
		// Graceful shutdown would be handled here
		exit(0);
	}
}

void print_usage(const char *prog)
{
	std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
						<< "Options:\n"
						<< "  -m, --model PATH       Path to GGUF model file (required)\n"
						<< "  -c, --config PATH      Path to config JSON file\n"
						<< "  -t, --threads N        Number of threads (default: 4)\n"
						<< "  -C, --ctx-size N       Context size (default: 2048)\n"
						<< "  -s, --socket PATH      Unix socket path (default: /tmp/forge-ai.sock)\n"
						<< "  -v, --verbose          Enable verbose logging\n"
						<< "  -h, --help             Show this help\n\n"
						<< "Example:\n"
						<< "  " << prog << " --model models/llama-3.2-3b-q4.gguf --threads 4\n";
}

int main(int argc, char **argv)
{
	// Default config
	LlamaConfig llm_config;
	llm_config.n_threads = 4;
	llm_config.n_ctx = 2048;
	llm_config.verbose = false;

	std::string socket_path = "/tmp/forge-ai.sock";
	std::string config_file;
	bool model_specified = false;

	// Parse command line arguments
	static struct option long_options[] = {
			{"model", required_argument, 0, 'm'},
			{"config", required_argument, 0, 'c'},
			{"threads", required_argument, 0, 't'},
			{"ctx-size", required_argument, 0, 'C'},
			{"socket", required_argument, 0, 's'},
			{"verbose", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}};

	int opt;
	int option_index = 0;

	while ((opt = getopt_long(argc, argv, "m:c:t:C:s:vh", long_options, &option_index)) != -1)
	{
		switch (opt)
		{
		case 'm':
			llm_config.model_path = optarg;
			model_specified = true;
			break;
		case 'c':
			config_file = optarg;
			break;
		case 't':
			llm_config.n_threads = std::atoi(optarg);
			break;
		case 'C':
			llm_config.n_ctx = std::atoi(optarg);
			break;
		case 's':
			socket_path = optarg;
			break;
		case 'v':
			llm_config.verbose = true;
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
		default:
			print_usage(argv[0]);
			return 1;
		}
	}

	// Load config file if specified
	if (!config_file.empty())
	{
		try
		{
			llm_config = LlamaConfig::from_file(config_file);
			std::cout << "[forge-runtime] Loaded config from: " << config_file << "\n";
		}
		catch (const std::exception &e)
		{
			std::cerr << "[ERROR] Failed to load config: " << e.what() << "\n";
			return 1;
		}
	}

	// Check if model is specified
	if (!model_specified && llm_config.model_path.empty())
	{
		std::cerr << "[ERROR] Model path is required. Use --model or --config\n\n";
		print_usage(argv[0]);
		return 1;
	}

	std::cout << "╔════════════════════════════════════════╗\n";
	std::cout << "║     Forge AI Runtime (with llama.cpp)  ║\n";
	std::cout << "╚════════════════════════════════════════╝\n\n";

	std::cout << "[Configuration]\n";
	std::cout << "  Model:       " << llm_config.model_path << "\n";
	std::cout << "  Threads:     " << llm_config.n_threads << "\n";
	std::cout << "  Context:     " << llm_config.n_ctx << " tokens\n";
	std::cout << "  Socket:      " << socket_path << "\n";
	std::cout << "  Verbose:     " << (llm_config.verbose ? "yes" : "no") << "\n\n";

	// Setup signal handlers
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	try
	{
		// 1. Initialize LLM Engine
		std::cout << "[1/4] Initializing LLM engine...\n";
		auto llm_engine = std::make_shared<LlamaEngine>(llm_config);

		std::cout << "[2/4] Loading model (this may take a few seconds)...\n";
		if (!llm_engine->load())
		{
			std::cerr << "[ERROR] Failed to load model\n";
			return 1;
		}

		std::cout << "  ✓ Model loaded: " << llm_engine->model_name() << "\n";
		std::cout << "  ✓ Context size: " << llm_engine->context_size() << " tokens\n";
		std::cout << "  ✓ Vocab size:   " << llm_engine->vocab_size() << " tokens\n\n";

		// 2. Register tools
		std::cout << "[3/4] Registering tools...\n";
		ToolRegistry registry;
		registry.register_tool(std::make_unique<ListDirTool>());
		// Add more tools here...

		std::cout << "  ✓ Registered " << registry.list().size() << " tool(s)\n\n";

		// 3. Create dispatcher
		std::cout << "[4/4] Starting IPC server...\n";
		ActionDispatcher dispatcher(registry, llm_engine);

		// 4. Start server
		SocketServer server(socket_path, dispatcher);
		g_server = &server;

		std::cout << "\n╔════════════════════════════════════════╗\n";
		std::cout << "║  Runtime ready! Listening for requests ║\n";
		std::cout << "╚════════════════════════════════════════╝\n\n";
		std::cout << "Press Ctrl+C to stop.\n\n";

		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "\n[FATAL ERROR] " << e.what() << "\n";
		return 1;
	}

	return 0;
}
