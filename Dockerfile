# syntax=docker/dockerfile:1.6
#
# Dockerfile for building snake-server on Linux (C++23, CMake 3.27+)
# Multi-stage: builder -> runtime
#
# Build:
#   docker build -t snake-server:latest .
#
# Run:
#   docker run --rm -p 9100:9100 -p 9000:9000/udp snake-server:latest
#
# Notes:
# - Project uses Boost (system,json), OpenSSL, MySQL client, SFML (system).
# - Server is built as a static-ish binary (BUILD_SHARED_LIBS OFF), but runtime
#   still needs some dynamic libs depending on toolchain / distro.
# - CMake minimum is 3.27 => we install a newer CMake via kitware APT repo.
#

############################
# 1) Builder stage
############################
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# --- Base build tools ---
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    gnupg \
    lsb-release \
    software-properties-common \
    wget \
    curl \
    git \
    build-essential \
    ninja-build \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# --- Install CMake 3.27+ (Kitware repo) ---
RUN set -eux; \
    wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc \
      | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg; \
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" \
      > /etc/apt/sources.list.d/kitware.list; \
    apt-get update; \
    apt-get install -y --no-install-recommends cmake; \
    rm -rf /var/lib/apt/lists/*

# --- Dependencies required by CMakeLists.txt ---
# Boost: system,json
# OpenSSL: SSL/Crypto
# MySQL: libmysqlclient-dev
# SFML: sfml-system (and deps)
# Extra: zlib, pthread is part of libc, etc.
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    libboost-system-dev \
    libboost-json-dev \
    libssl-dev \
    libmysqlclient-dev \
    libsfml-dev \
    && rm -rf /var/lib/apt/lists/*

# --- Workdir & sources ---
WORKDIR /src
COPY . /src

# --- Configure & build ---
# Project outputs binary into project root by CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_SOURCE_DIR}
# We keep that behavior and then copy the binary out.
RUN set -eux; \
    cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release;  \
    cmake --build build -j"$(nproc)"; \
    test -f /src/snake-server

############################
# 2) Runtime stage
############################
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Runtime libs:
# - OpenSSL runtime
# - libmysqlclient runtime
# - SFML runtime
# - Boost runtime (system/json)
# We install minimal runtime packages. If you fully static-link everything,
# you can reduce this list further.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libssl3 \
    libmysqlclient21 \
    libsfml-system2.6 \
    libboost-system1.83.0 \
    libboost-json1.83.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary
COPY --from=builder /src/snake-server /app/snake-server

# If your server requires configs/assets, copy them as well:
# COPY --from=builder /src/config /app/config

EXPOSE 9100/tcp
EXPOSE 7778/udp
EXPOSE 7779/udp
EXPOSE 7780/udp

# Recommended: run as non-root
RUN useradd -m -u 10001 appuser
USER appuser

ENTRYPOINT ["./snake-server"]