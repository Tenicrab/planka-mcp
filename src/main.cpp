#include <memory>
#include <string>
#include "transport/stdio.h"
#include "transport/http_sse.h"
#include "mcp/handler.h"
#include "planka/client.h"
#include "resources/registry.h"
#include "tools/registry.h"

int main(int argc, char* argv[]) {
    int port = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--http") {
            port = 8080;
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }

    auto client = PlankaClient::from_env();
    mcp::ResourceRegistry registry(client);
    mcp::ToolRegistry tool_registry(client);
    mcp::McpHandler handler(client, registry, tool_registry);

    std::unique_ptr<Transport> transport;
    if (port > 0) {
        transport = std::make_unique<mcp::HttpSseTransport>(port);
    } else {
        transport = std::make_unique<StdioTransport>();
    }

    transport->run([&](const std::string& msg) {
        return handler.handle_message(msg);
    });
    return 0;
}
