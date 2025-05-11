#!/bin/bash
# Script to generate C++ code from Protocol Buffer definitions
#run from root project directory
# Find the proto/ directory dynamically
PROTO_DIR=$(find "$(pwd)" -type d -name "proto" | head -n 1)

# Check if the proto/ directory exists
if [ -z "$PROTO_DIR" ]; then
    echo "Error: No directory named 'proto/' found."
    exit 1
fi

# Output directory for generated files
OUTPUT_DIR="$PROTO_DIR/generated"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Check if protoc and grpc_cpp_plugin are installed
if ! command -v protoc &> /dev/null; then
    echo "Error: protoc is not installed. Please install Protocol Buffers."
    exit 1
fi

if ! command -v grpc_cpp_plugin &> /dev/null; then
    echo "Error: grpc_cpp_plugin is not installed. Please install gRPC."
    exit 1
fi

# Generate C++ code for each .proto file in the proto/ directory
PROTO_FILES=("$PROTO_DIR"/*.proto)
if [ ${#PROTO_FILES[@]} -eq 0 ]; then
    echo "No .proto files found in $PROTO_DIR."
    exit 0
fi

for proto_file in "${PROTO_FILES[@]}"; do
    if [ -f "$proto_file" ]; then
        echo "Generating code for $proto_file"
        
        # Generate C++ code with gRPC service definitions
        protoc -I="$PROTO_DIR" --cpp_out="$OUTPUT_DIR" --grpc_out="$OUTPUT_DIR" \
            --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) "$proto_file"
        
        if [ $? -ne 0 ]; then
            echo "Error: Failed to generate code for $proto_file"
            exit 1
        fi
    fi
done

echo "Generation complete. Files are in $OUTPUT_DIR"