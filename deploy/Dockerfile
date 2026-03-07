# Stage 1: Build Environment
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libssl-dev \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Install xmake
RUN wget https://xmake.io/shget.text -O - | bash
ENV PATH="/root/.xmake/profile:${PATH}"
ENV PATH="/root/.local/bin:${PATH}"

# Setup project directory
WORKDIR /app
COPY . .

# Fetch dependencies and build statically/release (this downloads coke, wfrest, etc.)
RUN xmake f -m release --yes
RUN xmake build planka-mcp

# Stage 2: Minimal Runtime Environment
FROM ubuntu:24.04

# We still need libssl at runtime because wfrest/coke dynamically link to it
RUN apt-get update && apt-get install -y \
    ca-certificates \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary from the builder stage
COPY --from=builder /app/build/linux/x86_64/release/planka-mcp /usr/local/bin/planka-mcp

# Setup entrypoint environment variables
ENV PLANKA_URL=""
ENV PLANKA_TOKEN=""

# Expose default HTTP SSE port
EXPOSE 7526

ENTRYPOINT ["/usr/local/bin/planka-mcp"]
CMD ["--http", "--port", "7526"]
