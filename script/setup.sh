#!/usr/bin/env bash
set -euo pipefail

# ============================================================================
# Forge AI – Production Setup Script
# ============================================================================

PROJECT_NAME="Forge AI"
LLAMA_REPO_URL="https://github.com/ggerganov/llama.cpp.git"
LLAMA_DIR="third_party/llama.cpp"

MODEL_DIR="runtime/models"
MODEL_FILE="$MODEL_DIR/llama-3.2-3b-q4.gguf"
MODEL_URL="https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf"

# ----------------------------------------------------------------------------
# Colors
# ----------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------
fail() {
  echo -e "${RED}✗ $1${NC}"
  exit 1
}

info() {
  echo -e "${YELLOW}→ $1${NC}"
}

success() {
  echo -e "${GREEN}✓ $1${NC}"
}

# ----------------------------------------------------------------------------
# Preflight checks
# ----------------------------------------------------------------------------
echo ""
echo "==> $PROJECT_NAME setup"
echo ""

[[ -d "runtime" ]] || fail "Please run this script from the project root"

OS="$(uname -s)"

# ============================================================================
# 1. System dependencies
# ============================================================================
info "[1/3] Installing system dependencies"

if [[ "$OS" == "Linux" ]]; then
  command -v sudo >/dev/null || fail "sudo not found"
  sudo apt update
  sudo apt install -y \
    build-essential \
    cmake \
    git \
    socat \
    wget \
    nlohmann-json3-dev

elif [[ "$OS" == "Darwin" ]]; then
  command -v brew >/dev/null || fail "Homebrew not found (https://brew.sh)"
  brew update
  brew install \
    cmake \
    git \
    socat \
    wget \
    nlohmann-json
else
  fail "Unsupported OS: $OS"
fi

success "System dependencies installed"

# ============================================================================
# 2. llama.cpp setup (deterministic)
# ============================================================================
info "[2/3] Setting up llama.cpp"

mkdir -p third_party

# Always reset llama.cpp to avoid dirty states
if [[ -d "$LLAMA_DIR" ]]; then
  info "Removing existing llama.cpp"
  rm -rf "$LLAMA_DIR"
fi

info "Cloning llama.cpp (full repository)"
git clone --recurse-submodules "$LLAMA_REPO_URL" "$LLAMA_DIR"

# ----------------------------------------------------------------------------
# Verify llama.cpp integrity
# ----------------------------------------------------------------------------
info "Verifying llama.cpp integrity"

REQUIRED_FILES=(
  "include/llama.h"
  "src/llama.cpp"
  "ggml/include/ggml.h"
  "CMakeLists.txt"
)

for f in "${REQUIRED_FILES[@]}"; do
  [[ -f "$LLAMA_DIR/$f" ]] || fail "llama.cpp is incomplete (missing $f)"
done

# Verify ggml presence
if ! ls "$LLAMA_DIR" | grep -q '^ggml'; then
  fail "ggml sources not found in llama.cpp"
fi

success "llama.cpp verified and ready"

# ============================================================================
# 3. Model setup (optional)
# ============================================================================
info "[3/3] Model setup"

mkdir -p "$MODEL_DIR"

if [[ -f "$MODEL_FILE" ]]; then
  success "Model already exists ($(du -h "$MODEL_FILE" | cut -f1))"
else
  echo ""
  echo "Optional: download Llama 3.2 3B Q4_K_M model (~2GB)"
  read -p "Download now? [y/N] " -n 1 -r
  echo ""

  if [[ "$REPLY" =~ ^[Yy]$ ]]; then
    info "Downloading model (this may take a while)"

    if command -v wget >/dev/null; then
      wget --progress=bar:force:noscroll -O "$MODEL_FILE" "$MODEL_URL"
    else
      curl -L -o "$MODEL_FILE" "$MODEL_URL"
    fi

    [[ -f "$MODEL_FILE" ]] || fail "Model download failed"
    success "Model downloaded ($(du -h "$MODEL_FILE" | cut -f1))"
  else
    info "Skipping model download"
  fi
fi

# ============================================================================
# Summary
# ============================================================================
echo ""
echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   Setup complete (production-ready)   ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
echo ""

echo "Next steps:"
echo ""
echo "  cd runtime"
echo "  make build"
echo ""
echo "Then:"
echo "  make run"
echo ""
