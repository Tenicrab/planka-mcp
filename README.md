# Planka MCP (Model Context Protocol) Server v26.03.08

A high-performance C++20 based Model Context Protocol (MCP) server that connects large language model agents to Planka v2.

## Highlights

- **Full API Coverage**: Exposes 33+ meticulously mapped Planka v2 API actions to agents, including cascading operations, advanced enum verifications, and custom field handling.
- **Webhook Bridge**: Native support for Planka event tunneling. Agents can use the `subscribe_planka_events` tool to dynamically attach notification hooks that securely relay deep-nested Planka JSONs to the Agent's neural pathways via template rendering.
- **Asymmetric Precision Safety**: Solves Snowflake ID data destruction constraints globally by utilizing strict string-dump serialization to preserve 17+ digit Long Ints cleanly.
- **Progressive Sandbox**: Safe, test-driven validation sandbox using `coke` concurrent handling, preventing agent hallucinations scaling out of control.

## Building from Source

**Requirements:**

- Linux (Ubuntu 22.04/24.04 recommended)
- `xmake`
- A C++20 compliant compiler (GCC 11+)

```bash
# Clone the repository
git clone https://github.com/your-username/planka-mcp.git
cd planka-mcp

# Fetch dependencies and build statically (using xmake)
xmake f -m release --yes
xmake build planka-mcp
```

The compiled binary will be placed in `build/linux/x86_64/release/planka-mcp`.

## Running the Server

### Method 1: Stdio Mode (For local Agents like Claude Desktop)

Most AI clients run MCP plugins natively via standard input/output. You can point your AI client to execute the binary directly.

```json
{
  "mcpServers": {
    "planka": {
      "command": "/absolute/path/to/planka-mcp",
      "env": {
        "PLANKA_URL": "http://your-planka-domain/",
        "PLANKA_TOKEN": "YOUR_API_KEY_HERE"
      }
    }
  }
}
```

### Method 2: HTTP Webhook / SSE Mode (Port 7526)

If you wish to allow the MCP to receive Webhooks pushed directly from the Planka system, it must maintain a persistent, port-listening HTTP server state.

```bash
export PLANKA_URL="http://your-planka-domain/"
export PLANKA_TOKEN="YOUR_API_KEY_HERE"

./build/linux/x86_64/release/planka-mcp --http --port 7526
```

## Deployment Options

When running in HTTP mode (to receive Planka webhooks), the MCP Server needs to listen on **Port 7526**. You can deploy it natively or via Docker.

### Option A: Native Systemd (Highest Performance, No NAT headache)

We recommend running this natively if the MCP Server and Planka/Agent sit on the same physical routing layer.

1. Edit the provided template in `deploy/planka-mcp.service` so that `Environment` variables contain your credentials and `ExecStart` points to your built binary.
2. Install the service:

```bash
sudo cp deploy/planka-mcp.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable planka-mcp
sudo systemctl start planka-mcp
```

### Option B: Docker Deployment

If you prefer complete environment isolation, a multi-stage `Dockerfile` (based on Ubuntu 24.04 LTS) is provided.

**Build:**

```bash
docker build -t planka-mcp:latest -f deploy/Dockerfile .
```

**Run (Standard Port Mapping):**

```bash
docker run -d --name planka-mcp \
  -p 7526:7526 \
  -e PLANKA_URL="http://your-planka-domain/" \
  -e PLANKA_TOKEN="YOUR_API_KEY_HERE" \
  planka-mcp:latest
```

> **⚠️ CRITICAL: Webhook Isolation Notice!**  
> If you run the MCP in a standard Docker bridge (like above), and your Agent uses `subscribe_planka_events` to ask the MCP to forward notifications to `http://127.0.0.1:9090`, the MCP will fail! Inside the container, `127.0.0.1` refers to the container itself, not the host machine.
>
> **Solution 1 (Linux only):** Add `--network host` to strip Docker's network isolation.  
> **Solution 2 (Cross-platform):** Program your Agent to use `http://host.docker.internal:9090` instead of `127.0.0.1`.

## Security

Planka v2 restricts API access to Webhooks (`/api/webhooks`) and System configs strictly to accounts with the **ADMIN** role. It avoids sending an HTTP `403 Forbidden` and instead **cloaks the endpoint behind an HTTP `404 Not Found`**. Make sure your `PLANKA_TOKEN` belongs to an Admin account if you attempt to use those specific endpoints.
