#!/usr/bin/env bash
set -e

echo "==> Forge AI setup"

OS="$(uname -s)"

if [[ "$OS" == "Linux" ]]; then
	echo "Detected Linux"

	if ! command -v sudo &> /dev/null; then
		echo "sudo not found. Please run as root or install sudo."
		exit 1
	fi

	sudo apt update
	sudo apt install -y \
		build-essential \
		cmake \
		git \
		socat \
		nlohmann-json3-dev

elif [[ "$OS" == "Darwin" ]]; then
	echo "Detected macOS"

	if ! command -v brew &> /dev/null; then
		echo "Homebrew not found. Please install Homebrew first:"
		echo "https://brew.sh"
		exit 1
	fi

	brew update
	brew install \
		cmake \
		git \
		socat \
		nlohmann-json

else
	echo "Unsupported OS: $OS"
	exit 1
fi

echo
echo "==> Setup complete"
echo
echo "Next steps:"
echo "  make build"
echo "  make run"
echo
echo "Test with:"
echo "  echo '{ \"version\": 1, \"action\": \"ping\" }' | socat - UNIX-CONNECT:/tmp/forge-ai.sock"
