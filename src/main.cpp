#include <memory>
#include <string>
#include <vector>
#include "planka/client.h"
#include "mcp/handler.h"
#include "resources/registry.h"
#include "tools/registry.h"
#include "transport/stdio.h"
#include "transport/http_sse.h"
#include <logger.h>
#include "config.h"

int main(int argc, char* argv[]) {
    // Initialize maplog
    maplog::Logger::instance()
        .setLogDir("/tmp")
        .setFilePrefix("planka-mcp")
        .setConsoleOutput(true)
        .setConsoleLevel(maplog::LogLevel::INFO) // 生产环境：控制台仅输出 INFO 及以上级别
        .setFileLevel(maplog::LogLevel::DEBUG)  // 文件：保留 DEBUG 以供离线调试
        .init();

    LOG_INFO() << "planka-mcp v" << PLANKA_MCP_VERSION << " logger initialized.";
    // 默认配置
    bool http_mode = false;
    int port = 7526;
    PlankaClient::Config cmd_config;
    
    // 简易命令行解析
    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--http") {
            http_mode = true;
        } else if (args[i] == "--port" && i + 1 < args.size()) {
            port = std::stoi(args[++i]);
        } else if (args[i] == "--url" && i + 1 < args.size()) {
            cmd_config.url = args[++i];
        } else if (args[i] == "--api-key" && i + 1 < args.size()) {
            cmd_config.api_key = args[++i];
        }
    }

    // 初始化注册表 (不持有 Client 状态)
    mcp::ResourceRegistry registry;
    mcp::ToolRegistry tool_registry(registry);
    
    // 初始化无状态 Handler
    mcp::McpHandler handler(registry, tool_registry);

    // 传输层初始化
    std::unique_ptr<Transport> transport;
    if (http_mode) {
        transport = std::make_unique<mcp::HttpSseTransport>(port);
    } else {
        // Stdio 模式：如果命令行没给，尝试从环境变量拿（作为 Fallback）
        if (cmd_config.url.empty()) cmd_config.url = PlankaClient::get_env("PLANKA_URL");
        if (cmd_config.api_key.empty()) cmd_config.api_key = PlankaClient::get_env("PLANKA_API_KEY");
        
        transport = std::make_unique<StdioTransport>(cmd_config);
    }

    // 运行 MCP
    LOG_INFO() << "Starting planka-mcp in " << (http_mode ? "Server (SSE)" : "Stdio") << " mode...";
    
    transport->run([&handler](const std::string& request, const PlankaClient::Config& config) {
        return handler.handle_message(request, config);
    });

    return 0;
}
