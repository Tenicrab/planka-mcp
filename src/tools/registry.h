#pragma once
#include <string>
#include <vector>
#include <wfrest/Json.h>
#include <coke/coke.h>
#include "planka/client.h"
#include "mcp/types.h"

namespace mcp
{

class ResourceRegistry;

class ToolRegistry {
public:
    ToolRegistry(ResourceRegistry& resource_registry);

    // tools/list
    std::vector<Tool> list_tools();

    // tools/call
    coke::Task<wfrest::Json> call_tool(const std::string& name, const wfrest::Json& arguments, PlankaClient& client);

private:
    ResourceRegistry& resource_registry_;
    std::vector<ToolDef> definitions_;
    void init_definitions();

    // Helper to build schema from ToolDef
    wfrest::Json build_schema(const ToolDef& def);

    // Generic API caller based on ToolDef
    coke::Task<wfrest::Json> execute_generic(const ToolDef& def, const wfrest::Json& args, PlankaClient& client);
};

} // namespace mcp
