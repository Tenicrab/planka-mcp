# Stage 1: Build Environment
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV XMAKE_ROOT=y

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    zlib1g-dev \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Install xmake
RUN wget https://xmake.io/shget.text -O - | bash
ENV PATH="/root/.xmake/profile:${PATH}"
ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /app

# [Optimization] Copy ONLY configuration files first to cache dependency downloads
COPY xmake.lua .
COPY packages packages

# [Optimization] Trigger dependency download
RUN xmake f -m release --yes && xmake require --yes

# Copy the rest of the source code
COPY . .

# [Safety] Remove any accidental local cache copied from the host
RUN rm -rf .xmake build

# Final build and install
RUN xmake build --yes planka-mcp && \
    xmake install -o /app/dist planka-mcp

# Stage 2: Minimal Runtime Environment
FROM ubuntu:24.04

# Install runtime libraries and certificates
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary from the builder stage
COPY --from=builder /app/dist/bin/planka-mcp /usr/local/bin/planka-mcp

# Expose default HTTP SSE port
EXPOSE 7526

ENTRYPOINT ["/usr/local/bin/planka-mcp"]
CMD ["--http", "--port", "7526"]
