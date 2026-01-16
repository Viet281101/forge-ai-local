# ====== Config ======
BUILD_DIR := build
TARGET := forge_runtime
SOCKET := /tmp/forge-ai.sock

# Detect OS
UNAME_S := $(shell uname -s)

# ====== Default ======
.PHONY: all
all: build

# ====== Build ======
.PHONY: build
build:
	@echo "==> Building ($(UNAME_S))"
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

# ====== Run ======
.PHONY: run
run: build
	@echo "==> Running runtime"
	@rm -f $(SOCKET)
	./$(BUILD_DIR)/$(TARGET)

# ====== Clean ======
.PHONY: clean
clean:
	@echo "==> Cleaning build artifacts"
	rm -rf $(BUILD_DIR)
	rm -f $(SOCKET)

# ====== Rebuild ======
.PHONY: rebuild
rebuild: clean build

# ====== Test socket manually ======
.PHONY: test
test:
	@echo "==> Sending test request"
	echo '{ "ping": "pong" }' | socat - UNIX-CONNECT:$(SOCKET)
