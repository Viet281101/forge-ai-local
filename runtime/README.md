# Forge AI Runtime

An offline LLM runtime with tool calling capabilities, powered by llama.cpp.

## Features

- 🚀 **Offline LLM inference** - No API keys, no internet required
- 🛠️ **Tool calling** - AI can execute system tools automatically
- ⚡ **CPU optimized** - Runs efficiently on CPU using llama.cpp
- 🔌 **IPC interface** - JSON protocol over Unix sockets
- 🎯 **Extensible** - Easy to add custom tools

## System Requirements

- **OS:** Linux (Ubuntu 24.04+ recommended)
- **CPU:** x86_64 with AVX2 (Intel Core i5-6300U or better)
- **RAM:** 8GB minimum, 16GB recommended
- **Storage:** ~5GB for model + dependencies
- **Dependencies:** git, cmake, g++, make, wget/curl

## Quick Start

### 1. Setup (First Time)

```bash
cd runtime
chmod +x setup.sh
./setup.sh
```

This will:

- Initialize llama.cpp submodule
- Download a model (~2GB)
- Build the runtime

### 2. Run the Runtime

```bash
make run
```

You should see:

```
╔════════════════════════════════════════╗
║     Forge AI Runtime (with llama.cpp)  ║
╚════════════════════════════════════════╝

[Configuration]
  Model:       models/llama-3.2-3b-q4.gguf
  Threads:     4
  Context:     2048 tokens
  Socket:      /tmp/forge-ai.sock
  Verbose:     no

[1/4] Initializing LLM engine...
[2/4] Loading model (this may take a few seconds)...
  ✓ Model loaded: Llama 3.2 3B Instruct
  ✓ Context size: 2048 tokens
  ✓ Vocab size:   128256 tokens

╔════════════════════════════════════════╗
║  Runtime ready! Listening for requests ║
╚════════════════════════════════════════╝
```

### 3. Test It (In Another Terminal)

```bash
cd runtime

# Test basic functionality
make test-ping

# Test text generation
make test-generate

# Test AI-powered tool calling
make test-infer-ai
```

## Usage Examples

### Text Generation

```bash
echo '{
  "version": 1,
  "action": "generate",
  "prompt": "Write a Python function to reverse a string:",
  "max_tokens": 200,
  "temperature": 0.7
}' | socat - UNIX-CONNECT:/tmp/forge-ai.sock
```

### AI-Powered Inference (with tools)

```bash
echo '{
  "version": 1,
  "action": "infer",
  "messages": [
    {
      "role": "user",
      "content": "What files are in the current directory?"
    }
  ]
}' | socat - UNIX-CONNECT:/tmp/forge-ai.sock
```

The AI will automatically:

1. Decide it needs to use the `list_dir` tool
2. Execute the tool
3. Return a natural language response

### Get Model Info

```bash
echo '{
  "version": 1,
  "action": "model_info"
}' | socat - UNIX-CONNECT:/tmp/forge-ai.sock
```

## Available Actions

| Action       | Description                            |
| ------------ | -------------------------------------- |
| `ping`       | Health check                           |
| `generate`   | Raw text generation                    |
| `infer`      | AI-powered inference with tool calling |
| `list_tools` | List available tools                   |
| `model_info` | Get model information                  |

## Configuration

### Command Line Options

```bash
./build/forge_runtime [OPTIONS]

Options:
  -m, --model PATH       Path to GGUF model file (required)
  -c, --config PATH      Path to config JSON file
  -t, --threads N        Number of threads (default: 4)
  -C, --ctx-size N       Context size (default: 2048)
  -s, --socket PATH      Unix socket path (default: /tmp/forge-ai.sock)
  -v, --verbose          Enable verbose logging
  -h, --help             Show help
```

### Config File Example

Create `config.json`:

```json
{
	"model_path": "models/llama-3.2-3b-q4.gguf",
	"n_threads": 4,
	"n_ctx": 2048,
	"max_tokens": 512,
	"temperature": 0.7,
	"top_p": 0.9,
	"top_k": 40,
	"verbose": false
}
```

Run with config:

```bash
./build/forge_runtime --config config.json
```

## Recommended Models

For your Intel i5-6300U (4 threads, 15GB RAM):

| Model                   | Size  | RAM Usage | Performance | Best For                      |
| ----------------------- | ----- | --------- | ----------- | ----------------------------- |
| **Llama 3.2 3B Q4_K_M** | 2.0GB | ~3GB      | ~8-12 t/s   | General purpose (recommended) |
| **Phi-3 Mini Q4_K_M**   | 2.3GB | ~3.5GB    | ~7-10 t/s   | Coding & reasoning            |
| **Mistral 7B Q3_K_M**   | 3.3GB | ~4.5GB    | ~5-8 t/s    | More capable but slower       |
| **Gemma 2 2B Q4_K_M**   | 1.4GB | ~2.5GB    | ~12-15 t/s  | Fast, lightweight             |

Download models:

```bash
# Llama 3.2 3B (default)
make download-model

# Phi-3 Mini
make download-phi-3
```

## Performance Tuning

### For your i5-6300U:

```bash
# Optimal settings
./build/forge_runtime \
  --model models/llama-3.2-3b-q4.gguf \
  --threads 4 \
  --ctx-size 2048

# If RAM is limited
./build/forge_runtime \
  --model models/llama-3.2-3b-q4.gguf \
  --threads 4 \
  --ctx-size 1024  # Reduce context
```

### Benchmark

```bash
make benchmark
```

Expected performance:

- Llama 3.2 3B: ~8-12 tokens/second
- First token latency: ~80-120ms

## Makefile Commands

```bash
# Setup
make setup              # Complete setup
make download-model     # Download Llama 3.2 3B
make download-phi-3     # Download Phi-3 Mini

# Build
make build              # Build release
make build-debug        # Build debug
make rebuild            # Clean & rebuild

# Run
make run                # Run with Llama 3.2
make run-verbose        # Run with logs
make run-phi3           # Run with Phi-3

# Test
make test               # Run all tests
make test-generate      # Test generation
make test-infer-ai      # Test AI inference
make test-interactive   # Interactive mode

# Misc
make benchmark          # Performance test
make clean              # Clean build
make help               # Show all commands
```

## Adding Custom Tools

Create a new tool in `runtime/include/tools/`:

```cpp
// my_tool.h
#pragma once
#include "core/tool.h"

class MyTool : public Tool {
public:
    std::string name() const override { return "my_tool"; }

    std::string description() const override {
        return "Description of what my tool does";
    }

    json schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"param1", {
                    {"type", "string"},
                    {"description", "Parameter description"}
                }}
            }},
            {"required", {"param1"}}
        };
    }

    json run(const json &arguments) override {
        std::string param1 = arguments["param1"];
        // Your tool logic here
        return {{"result", "success"}};
    }
};
```

Register in `main.cpp`:

```cpp
#include "tools/my_tool.h"

// In main():
registry.register_tool(std::make_unique<MyTool>());
```

## Troubleshooting

### Model fails to load

```
[ERROR] Failed to load model
```

**Solutions:**

- Check model path: `ls -lh models/`
- Verify file integrity: Download again
- Check available RAM: `free -h`

### Out of memory

```
llama_kv_cache_init: failed to allocate buffer
```

**Solutions:**

- Reduce context size: `--ctx-size 1024`
- Use smaller model: Switch to Gemma 2B
- Close other applications

### Slow performance

**Solutions:**

- Check threads: Use `--threads $(nproc)`
- Verify model quantization: Use Q4_K_M or lighter
- Enable native optimizations (built by default)

### Socket connection error

```
socat: E connect: No such file or directory
```

**Solutions:**

- Ensure runtime is running
- Check socket path matches
- Verify permissions: `ls -l /tmp/forge-ai.sock`

## Architecture

```
┌─────────────────────────────────────────┐
│         Client Application              │
└───────────────┬─────────────────────────┘
                │ JSON over Unix Socket
┌───────────────▼─────────────────────────┐
│        SocketServer (IPC)               │
└───────────────┬─────────────────────────┘
                │
┌───────────────▼─────────────────────────┐
│      ActionDispatcher (Router)          │
│  ┌──────┬────────┬────────┬──────────┐  │
│  │ ping │ infer  │generate│list_tools│  │
│  └──────┴────────┴────────┴──────────┘  │
└───────┬──────────────────┬──────────────┘
        │                  │
  ┌─────▼──────┐    ┌──────▼──────────┐
  │ToolRegistry│    │  LlamaEngine    │
  │            │    │  (llama.cpp)    │
  │ - list_dir │    │                 │
  │ - ...      │    │ - generate()    │
  └────────────┘    │ - chat()        │
                    └─────────────────┘
```

## License

This project uses llama.cpp which is licensed under MIT License.

## Contributing

Contributions welcome! Please:

1. Fork the repo
2. Create a feature branch
3. Submit a pull request

## Support

For issues or questions:

- Open an issue on GitHub
- Check llama.cpp documentation
- Review existing issues

---

Built with ❤️ using llama.cpp
