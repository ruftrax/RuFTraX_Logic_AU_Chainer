#!/bin/bash
set -e
if [ -z "$JUCE_DIR" ]; then
  echo "Set JUCE_DIR first, example: export JUCE_DIR=/Users/mike/JUCE"
  exit 1
fi
cmake -B build -S . -DJUCE_DIR="$JUCE_DIR"
cmake --build build --config Release
