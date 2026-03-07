#pragma once
#include <string>
#include <optional>
#include "planka/client.h"

namespace mcp {

class ResourceRegistry;
class ToolRegistry;

class McpHandler {
public:
    McpHandler(PlankaClient& client, ResourceRegistry& registry, ToolRegistry& tool_registry);

    std::optional<std::string> handle_message(const std::string& json_str);

private:
    PlankaClient& client_;
    ResourceRegistry& registry_;
    ToolRegistry& tool_registry_;
};

} // namespace mcp
