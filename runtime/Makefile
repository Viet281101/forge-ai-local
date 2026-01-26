# Config
BUILD_DIR := build
TARGET := forge_runtime
SOCKET := /tmp/forge-ai.sock
MODEL_DIR := models
THIRD_PARTY := ../third_party

# URLs for models
LLAMA_3_2_3B_URL := https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf
PHI_3_MINI_URL := https://huggingface.co/bartowski/Phi-3-mini-128k-instruct-GGUF/resolve/main/Phi-3-mini-128k-instruct-Q4_K_M.gguf

# Detect OS and CPU cores
UNAME_S := $(shell uname -s)
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
NC := \033[0m # No Color

# ============================================================================
# Main targets
# ============================================================================

.PHONY: all
all: deps build

.PHONY: setup
setup: init-submodules download-model
	@echo "$(GREEN)==> Setup complete!$(NC)"

# ============================================================================
# Dependencies
# ============================================================================

.PHONY: init-submodules
init-submodules:
	@echo "$(YELLOW)==> Initializing git submodules$(NC)"
	@if [ ! -d "$(THIRD_PARTY)/llama.cpp/.git" ]; then \
		cd .. && git submodule update --init --recursive; \
	else \
		echo "$(GREEN)Submodules already initialized$(NC)"; \
	fi

.PHONY: deps
deps: init-submodules
	@echo "$(GREEN)==> Dependencies ready$(NC)"

# ============================================================================
# Model management
# ============================================================================

.PHONY: download-model
download-model: download-llama-3.2-3b

.PHONY: download-llama-3.2-3b
download-llama-3.2-3b:
	@echo "$(YELLOW)==> Downloading Llama 3.2 3B Q4_K_M (~2GB)$(NC)"
	@mkdir -p $(MODEL_DIR)
	@if [ ! -f "$(MODEL_DIR)/llama-3.2-3b-q4.gguf" ]; then \
		wget --progress=bar:force:noscroll -O $(MODEL_DIR)/llama-3.2-3b-q4.gguf $(LLAMA_3_2_3B_URL) || \
		curl -L -o $(MODEL_DIR)/llama-3.2-3b-q4.gguf $(LLAMA_3_2_3B_URL); \
		echo "$(GREEN)Downloaded successfully$(NC)"; \
	else \
		echo "$(GREEN)Model already exists$(NC)"; \
	fi

.PHONY: download-phi-3
download-phi-3:
	@echo "$(YELLOW)==> Downloading Phi-3 Mini Q4_K_M (~2.3GB)$(NC)"
	@mkdir -p $(MODEL_DIR)
	@if [ ! -f "$(MODEL_DIR)/phi-3-mini-q4.gguf" ]; then \
		wget --progress=bar:force:noscroll -O $(MODEL_DIR)/phi-3-mini-q4.gguf $(PHI_3_MINI_URL) || \
		curl -L -o $(MODEL_DIR)/phi-3-mini-q4.gguf $(PHI_3_MINI_URL); \
		echo "$(GREEN)Downloaded successfully$(NC)"; \
	else \
		echo "$(GREEN)Model already exists$(NC)"; \
	fi

.PHONY: list-models
list-models:
	@echo "$(YELLOW)==> Available models in $(MODEL_DIR):$(NC)"
	@if [ -d "$(MODEL_DIR)" ]; then \
		ls -lh $(MODEL_DIR)/*.gguf 2>/dev/null || echo "No models found"; \
	else \
		echo "No models directory"; \
	fi

# ============================================================================
# Build
# ============================================================================

.PHONY: build
build: deps
	@echo "$(YELLOW)==> Building ($(UNAME_S)) with $(NPROC) cores$(NC)"
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) -j$(NPROC)
	@echo "$(GREEN)==> Build complete!$(NC)"

.PHONY: build-debug
build-debug: deps
	@echo "$(YELLOW)==> Building DEBUG version$(NC)"
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) -j$(NPROC)

# ============================================================================
# Run
# ============================================================================

.PHONY: run
run: build
	@echo "$(YELLOW)==> Starting runtime$(NC)"
	@rm -f $(SOCKET)
	@if [ -f "$(MODEL_DIR)/llama-3.2-3b-q4.gguf" ]; then \
		./$(BUILD_DIR)/$(TARGET) --model $(MODEL_DIR)/llama-3.2-3b-q4.gguf --threads $(NPROC); \
	else \
		echo "$(RED)ERROR: Model not found. Run 'make download-model' first$(NC)"; \
		exit 1; \
	fi

.PHONY: run-verbose
run-verbose: build
	@echo "$(YELLOW)==> Starting runtime (verbose mode)$(NC)"
	@rm -f $(SOCKET)
	./$(BUILD_DIR)/$(TARGET) --model $(MODEL_DIR)/llama-3.2-3b-q4.gguf --threads $(NPROC) --verbose

.PHONY: run-phi3
run-phi3: build
	@echo "$(YELLOW)==> Starting runtime with Phi-3$(NC)"
	@rm -f $(SOCKET)
	@if [ -f "$(MODEL_DIR)/phi-3-mini-q4.gguf" ]; then \
		./$(BUILD_DIR)/$(TARGET) --model $(MODEL_DIR)/phi-3-mini-q4.gguf --threads $(NPROC); \
	else \
		echo "$(RED)ERROR: Phi-3 model not found. Run 'make download-phi-3' first$(NC)"; \
		exit 1; \
	fi

# ============================================================================
# Test
# ============================================================================

.PHONY: test
test: test-ping test-model-info test-generate test-infer-ai

.PHONY: test-ping
test-ping:
	@echo "$(YELLOW)==> Test: Ping$(NC)"
	@echo '{"version":1,"action":"ping"}' | socat - UNIX-CONNECT:$(SOCKET)
	@echo ""

.PHONY: test-model-info
test-model-info:
	@echo "$(YELLOW)==> Test: Model Info$(NC)"
	@echo '{"version":1,"action":"model_info"}' | socat - UNIX-CONNECT:$(SOCKET)
	@echo ""

.PHONY: test-list-tools
test-list-tools:
	@echo "$(YELLOW)==> Test: List Tools$(NC)"
	@echo '{"version":1,"action":"list_tools"}' | socat - UNIX-CONNECT:$(SOCKET)
	@echo ""

.PHONY: test-generate
test-generate:
	@echo "$(YELLOW)==> Test: Text Generation$(NC)"
	@printf '{"version":1,"action":"generate","prompt":"Write a Python function to calculate fibonacci:\\n\\ndef fib(n):","max_tokens":200,"temperature":0.7}' | socat -T60 - UNIX-CONNECT:$(SOCKET)
	@echo ""

.PHONY: test-infer-ai
test-infer-ai:
	@echo "$(YELLOW)==> Test: AI-powered tool calling$(NC)"
	@printf '{"version":1,"action":"infer","messages":[{"role":"user","content":"What files are in the current directory?"}],"max_tokens":300}' | socat -T60 - UNIX-CONNECT:$(SOCKET)
	@echo ""

.PHONY: test-infer-explicit
test-infer-explicit:
	@echo "$(YELLOW)==> Test: Explicit tool call$(NC)"
	@echo '{"version":1,"action":"infer","messages":[{"role":"assistant","tool_calls":[{"id":"call_1","function":{"name":"list_dir","arguments":{"path":"."}}}]}]}' | socat - UNIX-CONNECT:$(SOCKET)
	@echo ""

.PHONY: test-interactive
test-interactive:
	@echo "$(YELLOW)==> Interactive mode (Ctrl+C to exit)$(NC)"
	@echo "Enter JSON requests:"
	socat - UNIX-CONNECT:$(SOCKET)

# ============================================================================
# Benchmark
# ============================================================================

.PHONY: benchmark
benchmark: build
	@echo "$(YELLOW)==> Running performance benchmark$(NC)"
	@echo '{"version":1,"action":"generate","prompt":"The quick brown fox","max_tokens":100}' | socat - UNIX-CONNECT:$(SOCKET) | grep -o '"tokens_per_second":[0-9.]*' || true

# ============================================================================
# Clean
# ============================================================================

.PHONY: clean
clean:
	@echo "$(YELLOW)==> Cleaning build artifacts$(NC)"
	rm -rf $(BUILD_DIR)
	rm -f $(SOCKET)
	@echo "$(GREEN)==> Clean complete$(NC)"

.PHONY: clean-all
clean-all: clean
	@echo "$(YELLOW)==> Cleaning everything including models$(NC)"
	rm -rf $(MODEL_DIR)

.PHONY: rebuild
rebuild: clean build

# ============================================================================
# Help
# ============================================================================

.PHONY: help
help:
	@echo "$(GREEN)Forge AI Runtime - Makefile Commands$(NC)"
	@echo ""
	@echo "$(YELLOW)Setup:$(NC)"
	@echo "  make setup              - Complete setup (init submodules + download model)"
	@echo "  make init-submodules    - Initialize llama.cpp submodule"
	@echo "  make download-model     - Download default model (Llama 3.2 3B)"
	@echo "  make download-phi-3     - Download Phi-3 Mini model"
	@echo "  make list-models        - Show downloaded models"
	@echo ""
	@echo "$(YELLOW)Build:$(NC)"
	@echo "  make build              - Build release version"
	@echo "  make build-debug        - Build debug version"
	@echo "  make rebuild            - Clean and rebuild"
	@echo ""
	@echo "$(YELLOW)Run:$(NC)"
	@echo "  make run                - Run with Llama 3.2 3B"
	@echo "  make run-verbose        - Run with verbose logging"
	@echo "  make run-phi3           - Run with Phi-3 model"
	@echo ""
	@echo "$(YELLOW)Test:$(NC)"
	@echo "  make test               - Run all tests"
	@echo "  make test-ping          - Test ping action"
	@echo "  make test-model-info    - Test model info"
	@echo "  make test-generate      - Test text generation"
	@echo "  make test-infer-ai      - Test AI-powered inference"
	@echo "  make test-interactive   - Interactive testing"
	@echo "  make benchmark          - Run performance benchmark"
	@echo ""
	@echo "$(YELLOW)Clean:$(NC)"
	@echo "  make clean              - Remove build artifacts"
	@echo "  make clean-all          - Remove everything including models"
	@echo ""
	@echo "$(YELLOW)Quick Start:$(NC)"
	@echo "  1. make setup           - First time setup"
	@echo "  2. make run             - Start the runtime"
	@echo "  3. make test            - Test in another terminal"
