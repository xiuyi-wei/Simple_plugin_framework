#!/bin/bash

# Clean previous build
rm -rf build
# Configure the project
cmake . -B build
# Build the project
cmake --build build -j$(nproc)
