#!/usr/bin/env bash
set -e

echo "==> Forge AI setup"

OS="$(uname -s)"

if [[ "$OS" == "Linux" ]]; then
	echo "Detected Linux"

	sudo apt update
	sudo apt install -y \
		build-essential \
		cmake \
		make \
		git \
		socat

elif [[ "$OS" == "Darwin" ]]; then
	echo "Detected macOS"

	if ! command -v brew &> /dev/null; then
		echo "Homebrew not found. Please install Homebrew first:"
		echo "https://brew.sh"
		exit 1
	fi

	brew install \
		cmake \
		make \
		git \
		socat

else
	echo "Unsupported OS: $OS"
	exit 1
fi

echo "==> Setup complete"
