#!/bin/bash
set -e
if [ -z "$JUCE_DIR" ]; then
  if [ -f "$HOME/JUCE/CMakeLists.txt" ]; then
    JUCE_DIR="$HOME/JUCE"
  else
    echo "JUCE not found -- cloning it now (one-time, ~1GB)..."
    command -v git >/dev/null || { echo "git is required. Install the Xcode Command Line Tools first: xcode-select --install"; exit 1; }
    git clone --depth 1 --branch 8.0.14 https://github.com/juce-framework/JUCE.git "$HOME/JUCE"
    JUCE_DIR="$HOME/JUCE"
  fi
fi
command -v cmake >/dev/null || { if command -v brew >/dev/null; then echo "cmake not found -- installing it now via Homebrew..."; brew install cmake; else echo "cmake is required. Install it with: brew install cmake"; exit 1; fi; }
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR="$JUCE_DIR"
cmake --build build --config Release
