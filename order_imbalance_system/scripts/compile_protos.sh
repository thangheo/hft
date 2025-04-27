#!/bin/bash
# Compile Protocol Buffers

# Ensure we're in the project root
PROJECT_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
cd "$PROJECT_ROOT"

# Create output directories if they don't exist
mkdir -p include/protos
mkdir -p src/protos
mkdir -p python/protos

# Compile C++ protos
for proto_file in protos/*.proto; do
  echo "Compiling C++ for $proto_file..."
  protoc -I=protos --cpp_out=src/protos "$proto_file"
  protoc -I=protos --cpp_out=include/protos "$proto_file"
done

# Compile Python protos
for proto_file in protos/*.proto; do
  echo "Compiling Python for $proto_file..."
  protoc -I=protos --python_out=python/protos "$proto_file"
done

# Create Python __init__.py
touch python/protos/__init__.py

echo "Protocol Buffers compilation finished!"