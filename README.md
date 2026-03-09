# Planka MCP Server 🚀
> The most powerful, decentralized bridge between AI Agents and Planka v2.

[![Language](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/License-Fair%20Use-green.svg)](https://github.com/plankanban/planka/blob/master/LICENSE.md)

This high-performance MCP server transforms your Planka instance into a structured knowledge base for AI Assistants (Claude, Cursor, etc.). 

## 🌟 Key Features

- **Progressive Discovery**: Instead of overwhelming your AI with 60+ tools, we use **Dynamic Resources** (`planka://hub`, `planka://projects`) to guide the Agent. It only sees what it needs, when it needs it.
- **The "God Hub"**: Start with `planka://hub` for a global overview of your entire Planka instance—projects, users, and unread notifications at a glance.
- **Decentralized Notifications**: Custom `Notification Services` support. Non-admin users can now build their own automation flows by bridging Planka events to local receivers via reverse-proxy (frp).
- **Stateless & Secure**: Zero persistence on the server side. Credentials are injected per-request by the Agent, ensuring absolute privacy and multi-tenant support.
- **Native C++20 Performance**: Built with `coke` and `wfrest`, leveraging high-concurrency coroutines for zero-latency interactions.

---

## ⚙️ Configuration (mcpServers.json)

Planka-MCP adapts to your environment: **Stdio** for local desktop use, and **SSE** for remote/Docker deployment.

### 1. Stdio Mode (CLI Driven)
Ideal for **Claude Desktop** or **Cursor**. Credential injection is handled via arguments.

```json
{
  "mcpServers": {
    "planka": {
      "command": "/usr/local/bin/planka-mcp",
      "args": [
        "--stdio",
        "--url", "https://your-planka-instance.top:8080",
        "--api-key", "HwQsvOKO_YOUR_SECRET_KEY"
      ]
    }
  }
}
```

### 2. SSE Mode (Gateway Driven)
Ideal for **Docker** or cloud deployment. The server acts as a stateless bridge.

```json
{
  "mcpServers": {
    "planka": {
      "url": "http://your-gateway-ip:7526/",
      "headers": {
        "X-Planka-Url": "https://your-planka-instance.top:8080",
        "X-Planka-Api-Key": "HwQsvOKO_YOUR_SECRET_KEY"
      }
    }
  }
}
```

---

## 🏗️ Getting the Binary

### Option A: Build from Source

**Environment:** Linux (Ubuntu 24.04), C++20 compiler.

1. **Install xmake**:
   ```bash
   curl -fsSL https://xmake.io/shget.text | bash
   # source ~/.bashrc 
   ```

2. **Clone & Build**:
   ```bash
   git clone https://github.com/Tenicrab/planka-mcp.git
   cd planka-mcp

   # Build and Install to ./dist
   xmake f -m release --yes
   xmake build planka-mcp
   xmake install -o ./dist planka-mcp
   ```
   Binary: `./dist/bin/planka-mcp`.

### Option B: Docker Deployment
No secrets are stored in the image. Credentials are injected by the Agent via Headers or Args.

**Recommended for IPv6 stability and proper signal handling:**
```bash
docker build -t planka-mcp:latest -f deploy/Dockerfile .

# Run with --network host for native IPv6 support and --init for Ctrl+C support
docker run -d --name planka-mcp \
  --network host \
  --init \
  --restart always \
  planka-mcp:latest --http --port 7526
```

---

## 🔒 Stateless Security

- **Agent-Driven**: This server follows the **Principle of Agent-Controlled Configuration**. It does not persist credentials.
- **Multitenancy**: A single server instance can safely serve multiple users with different Planka backends by using separate headers for each client connection.
- **Precision**: Enforces string-based Snowflake ID handling to prevent precision loss in LLM interactions.

## ⚠️ Networking Note
When running in Docker and forwarding events back to a local Agent, use `http://host.docker.internal:PORT` to bridge container isolation.
