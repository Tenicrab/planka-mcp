#pragma once
#include <string>
#include <optional>
#include "planka/client.h"

namespace mcp {

class ResourceRegistry;
class ToolRegistry;

class McpHandler {
public:
    McpHandler(ResourceRegistry& registry, ToolRegistry& tool_registry);

    std::optional<std::string> handle_message(const std::string& json_str, const PlankaClient::Config& config);

private:
    ResourceRegistry& registry_;
    ToolRegistry& tool_registry_;
};

} // namespace mcp
