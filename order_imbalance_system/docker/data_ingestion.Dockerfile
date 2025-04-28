# docker/data_ingestion.Dockerfile
FROM debian:bullseye-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libssl-dev \
    libzmq3-dev \
    pkg-config \
    protobuf-compiler \
    libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy source code
COPY include/ /app/include/
COPY src/ /app/src/
COPY CMakeLists.txt /app/

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j $(nproc)

# Run the application
CMD ["./build/bin/data_ingestion"]
